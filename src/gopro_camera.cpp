// ============================================================================
// gopro_camera.cpp — Open GoPro BLE client (HERO8 Black 2019 and newer)
// ============================================================================
// Wire protocol reference: https://gopro.github.io/OpenGoPro/
//
// ─── GATT layout (base UUID b5f9XXXX-aa8d-11e3-9046-0002a5d5c51b) ───────────
//   Service FEA6 "Control & Query"
//     GP-0072 Command            (write)
//     GP-0073 Command Response   (notify)
//     GP-0074 Settings           (write)   ← keep-alive
//     GP-0075 Settings Response  (notify)
//     GP-0076 Query              (write)   ← status registration / polling
//     GP-0077 Query Response     (notify)  ← battery %, encoding state
//
// ─── Frames ─────────────────────────────────────────────────────────────────
//   Command request : [0x03][payloadLen][payload…]
//   Setting request : [0x02][payloadLen][payload…]
//   Query   request : [0x01][payloadLen][payload…]
//   Shutter ON      : 03 1A {%230%22shutter%22%3Atrue}
//   Shutter OFF     : 03 1B {%230%22shutter%22%3Afalse}
//   Keep-alive      : 02 01 42                       (every ~3 s)
//   Register status : 01 04 53 0A 0D 46
//                     (0x53=register, ids: 10 Encoding, 13 Rec Duration,
//                      70 Internal Battery Percentage)
//
// ─── Pairing ────────────────────────────────────────────────────────────────
//   First connection requires tapping "Pair" on the camera screen.  We bond,
//   so subsequent connections are automatic.  Camera must be in pairing /
//   connectable mode (Connections → Wireless Connections → GoPro Quik).
// ============================================================================

#include "gopro_camera.h"
#include "cam_registry.h"
#include "scan_results.h"
#include <NimBLEDevice.h>

// ──────────────────────────────────────────────────────────────────────────────
// GATT UUIDs
// ──────────────────────────────────────────────────────────────────────────────

static NimBLEUUID GP_SERVICE_UUID("0000fea6-0000-1000-8000-00805f9b34fb");
static NimBLEUUID GP_CHAR_COMMAND("b5f90072-aa8d-11e3-9046-0002a5d5c51b");
static NimBLEUUID GP_CHAR_CMD_RESPONSE("b5f90073-aa8d-11e3-9046-0002a5d5c51b");
static NimBLEUUID GP_CHAR_SETTINGS("b5f90074-aa8d-11e3-9046-0002a5d5c51b");
static NimBLEUUID GP_CHAR_QUERY("b5f90076-aa8d-11e3-9046-0002a5d5c51b");
static NimBLEUUID GP_CHAR_QUERY_RESPONSE("b5f90077-aa8d-11e3-9046-0002a5d5c51b");

// Registered status IDs (all uint8 values)
#define GP_STATUS_BATTERY_BARS   0x02   // enum 0..4 (fallback if % unsupported)
#define GP_STATUS_ENCODING       0x0A   // 1 = recording
#define GP_STATUS_BATTERY_PCT    0x46   // 0..100 %

// ──────────────────────────────────────────────────────────────────────────────
// Internal state
// ──────────────────────────────────────────────────────────────────────────────

static BleConnectionState _bleState      = BLE_DISCONNECTED;
static CameraTelemetry    _telemetry;

static NimBLEClient             *_pClient        = nullptr;
static NimBLERemoteCharacteristic *_pCommandChar = nullptr;
static NimBLERemoteCharacteristic *_pSettingsChar = nullptr;
static NimBLERemoteCharacteristic *_pQueryChar    = nullptr;

static NimBLEAddress _targetAddress;
static bool          _hasTargetAddress = false;
static bool          _doConnect        = false;

static uint32_t _lastReconnectAttempt = 0;
static uint32_t _lastKeepAlive        = 0;
static uint32_t _lastStatusPoll       = 0;
static uint32_t _connectedAtMs        = 0;

static bool _registeredStatuses       = false;

// Local rec-time clock: counts while the camera reports encoding.
static uint32_t _recStartMs           = 0;

// ──────────────────────────────────────────────────────────────────────────────
// Forward declarations
// ──────────────────────────────────────────────────────────────────────────────

static void startScan();
static bool connectToCamera();
static void notifyCallback(NimBLERemoteCharacteristic *pChar, uint8_t *pData,
                           size_t length, bool isNotify);
static void parseQueryResponse(const uint8_t *data, size_t len);
static bool isGoProDevice(NimBLEAdvertisedDevice *device);
static void sendKeepAlive();
static void sendRegisterStatusUpdates();
static void sendStatusQuery();

// ──────────────────────────────────────────────────────────────────────────────
// BLE Callbacks
// ──────────────────────────────────────────────────────────────────────────────

class GpClientCallbacks : public NimBLEClientCallbacks {
    void onConnect(NimBLEClient *pClient) override {
        DBG("GP: Connected to camera");
    }
    void onDisconnect(NimBLEClient *pClient) {
        DBG("GP: Disconnected from camera");
        _bleState = BLE_DISCONNECTED;
        _pCommandChar = nullptr;
        _pSettingsChar = nullptr;
        _pQueryChar = nullptr;
        _registeredStatuses = false;
        _telemetry.dataValid = false;
        _telemetry.state = CAM_STATE_UNKNOWN;
        _recStartMs = 0;
    }
};
static GpClientCallbacks _clientCallbacks;

// ──────────────────────────────────────────────────────────────────────────────
// RTOS-SAFE scan callback. Runs on the NimBLE host task — any blocking call
// (Preferences, Serial.printf, snprintf to a long string) will starve the
// BLE stack and trip the RTOS watchdog.
// Rules enforced below:
//   • NO Preferences/NVS writes (deferred to camRegistryProcess() in loop()).
//   • NO DBG/Serial output (the snprintf to a 128B buffer can take ms).
//   • Only lightweight RAM pushes to lock-free pending slots.
// ──────────────────────────────────────────────────────────────────────────────
class GpScanCallbacks : public NimBLEAdvertisedDeviceCallbacks {
    void onResult(NimBLEAdvertisedDevice *advertisedDevice) override {
        if (isGoProDevice(advertisedDevice)) {
            const char *mac = advertisedDevice->getAddress().toString().c_str();
            int8_t rssi   = advertisedDevice->getRSSI();
            const char *name = "";

            // Lock-free RAM push: scan_results module stores it in a static
            // array with no I/O. NVS write happens later in loop().
            scanResultsAdd(CAMERA_GOPRO, mac, name, rssi);

            // Pending registry write: just set a flag + copy into a small
            // struct. No NVS, no allocation, no std::string persistence.
            camRegistryRemember(CAMERA_GOPRO, mac, name);

            // Auto-connect check: read-only scan of the saved list.
            if (camRegistryMayAutoConnect(CAMERA_GOPRO, mac)) {
                NimBLEDevice::getScan()->stop();
                _targetAddress    = advertisedDevice->getAddress();
                _hasTargetAddress = true;
                _doConnect        = true;
            }
        }
    }
};
static GpScanCallbacks _scanCallbacks;

// ──────────────────────────────────────────────────────────────────────────────
// Device identification — GoPros advertise service 0xFEA6 and/or a name like
// "GoPro 1234" / legacy "GP12345678".
// ──────────────────────────────────────────────────────────────────────────────

static NimBLEUUID GOPRO_ADV_SERVICE((uint16_t)0xFEA6);

static bool isGoProDevice(NimBLEAdvertisedDevice *device) {
    if (device->isAdvertisingService(GOPRO_ADV_SERVICE)) return true;
    if (device->haveName()) {
        std::string name = device->getName();
        if (name.rfind("GoPro", 0) == 0 || name.rfind("GOPRO", 0) == 0 ||
            name.rfind("GP", 0) == 0) {
            return true;
        }
    }
    return false;
}

// ──────────────────────────────────────────────────────────────────────────────
// Scan management
// ──────────────────────────────────────────────────────────────────────────────

/// No-op completion handler — required so NimBLE uses the *non-blocking*
/// scan API (the two-argument overload blocks the main loop forever when
/// duration == 0).
static void scanCompleteCb(NimBLEScanResults results) {
    DBG("GP: Scan window complete (%d devices)", results.getCount());
    scanResultsMarkComplete();
    scanResultsUpdateSavedStatus();
}

static void startScan() {
    if (_bleState == BLE_SCANNING) return;
    DBG("GP: Starting 5s scan (40%% duty cycle)...");
    _bleState  = BLE_SCANNING;
    _doConnect = false;

    scanResultsClear();
    camRegistryClearDiscovered();   // New scan: wipe in-RAM discovered list

    NimBLEScan *pScan = NimBLEDevice::getScan();
    pScan->setAdvertisedDeviceCallbacks(&_scanCallbacks, false);
    pScan->setActiveScan(true);
    // ── CRITICAL: 40ms window in 100ms interval = 40% radio duty cycle.
    //    Leaves 60% airtime for the Wi-Fi SoftAP to keep beaconing.
    pScan->setInterval(100);
    pScan->setWindow(40);
    pScan->setMaxResults(0);
    // Non-blocking 5-second window. The main loop will re-call startScan()
    // after the completion callback to keep discovery continuous.
    pScan->start(5, false);
}

// ──────────────────────────────────────────────────────────────────────────────
// Connection & GATT discovery
// ──────────────────────────────────────────────────────────────────────────────

static bool connectToCamera() {
    if (!_hasTargetAddress) return false;
    DBG("GP: Connecting to %s...", _targetAddress.toString().c_str());
    _bleState = BLE_CONNECTING;

    if (!_pClient) {
        _pClient = NimBLEDevice::createClient();
        _pClient->setClientCallbacks(&_clientCallbacks, false);
        _pClient->setConnectTimeout(BLE_CONNECT_TIMEOUT_MS / 1000);
    }

    if (!_pClient->connect(_targetAddress)) {
        DBG("GP: Connection failed!");
        _bleState = BLE_DISCONNECTED;
        return false;
    }

    DBG("GP: Discovering GATT services...");
    NimBLERemoteService *pService = _pClient->getService(GP_SERVICE_UUID);
    if (!pService) {
        DBG("GP: Service 0xFEA6 not found! Disconnecting.");
        _pClient->disconnect();
        _bleState = BLE_DISCONNECTED;
        return false;
    }

    _pCommandChar  = pService->getCharacteristic(GP_CHAR_COMMAND);
    _pSettingsChar = pService->getCharacteristic(GP_CHAR_SETTINGS);
    _pQueryChar    = pService->getCharacteristic(GP_CHAR_QUERY);

    NimBLERemoteCharacteristic *pCmdResp  = pService->getCharacteristic(GP_CHAR_CMD_RESPONSE);
    NimBLERemoteCharacteristic *pQueryRsp = pService->getCharacteristic(GP_CHAR_QUERY_RESPONSE);

    if (!_pCommandChar || !_pSettingsChar || !_pQueryChar) {
        DBG("GP: Missing required characteristics!");
        _pClient->disconnect();
        _bleState = BLE_DISCONNECTED;
        return false;
    }

    if (pCmdResp && pCmdResp->canNotify()) {
        pCmdResp->subscribe(true, notifyCallback);
        DBG("GP: Subscribed to command responses");
    }
    if (pQueryRsp && pQueryRsp->canNotify()) {
        pQueryRsp->subscribe(true, notifyCallback);
        DBG("GP: Subscribed to query responses");
    }

    strlcpy(_telemetry.model, "GoPro HERO", sizeof(_telemetry.model));

    // Initiate LE pairing/bonding after discovery & subscriptions are done
    // (camera shows the approval prompt on first run; bonds make later
    // connections silent).
    _pClient->secureConnection();

    // Considered connected once GATT is up; pairing approval happens async.
    // If the user hasn't approved yet, commands are ignored by the camera and
    // our desired-state logic will re-send them once telemetry flows.
    _bleState       = BLE_CONNECTED;
    _connectedAtMs  = millis();
    _lastKeepAlive  = 0;         // Send keep-alive immediately
    _lastStatusPoll = 0;

    sendKeepAlive();
    sendRegisterStatusUpdates();
    return true;
}

// ──────────────────────────────────────────────────────────────────────────────
// Protocol helpers
// ──────────────────────────────────────────────────────────────────────────────

/// Write to the Command characteristic (with response).
static bool writeCommand(const uint8_t *data, size_t len) {
    if (_pCommandChar && _pCommandChar->canWrite()) {
        _pCommandChar->writeValue((uint8_t *)data, len, true);
        return true;
    }
    return false;
}

static void sendShutter(bool on) {
    // Encoded JSON payloads per Open GoPro data protocol.
    static const char SHUTTER_ON[]  = "{%230%22shutter%22%3Atrue}";
    static const char SHUTTER_OFF[] = "{%230%22shutter%22%3Afalse}";

    const char *payload = on ? SHUTTER_ON : SHUTTER_OFF;
    uint8_t plen = strlen(payload);

    uint8_t frame[40];
    frame[0] = 0x03;         // Command request header
    frame[1] = plen;
    memcpy(&frame[2], payload, plen);

    if (writeCommand(frame, plen + 2)) {
        DBG("GP: Sent shutter=%s", on ? "ON" : "OFF");
    } else {
        DBG("GP: Failed to send shutter command");
    }
}

static void sendKeepAlive() {
    if (!_pSettingsChar || !_pSettingsChar->canWrite()) return;
    const uint8_t ka[3] = {0x02, 0x01, 0x42};  // Setting request | len 1 | 0x42
    _pSettingsChar->writeValue((uint8_t *)ka, sizeof(ka), true);
}

static void sendRegisterStatusUpdates() {
    if (!_pQueryChar || !_pQueryChar->canWrite()) return;
    // [0x01|len|0x53 register|ids…] — Encoding + Battery %
    const uint8_t reg[5] = {0x01, 0x03, 0x53,
                            GP_STATUS_ENCODING, GP_STATUS_BATTERY_PCT};
    _pQueryChar->writeValue((uint8_t *)reg, sizeof(reg), true);
    _registeredStatuses = true;
    DBG("GP: Registered status updates");
}

static void sendStatusQuery() {
    if (!_pQueryChar || !_pQueryChar->canWrite()) return;
    // Get Status Values (id 0x13) for the same set of statuses.
    const uint8_t q[5] = {0x01, 0x03, 0x13,
                          GP_STATUS_ENCODING, GP_STATUS_BATTERY_PCT};
    _pQueryChar->writeValue((uint8_t *)q, sizeof(q), true);
}

// ──────────────────────────────────────────────────────────────────────────────
// Query response parsing
// ──────────────────────────────────────────────────────────────────────────────
// Notifications arrive as [header][len?][entries…].  Entry layouts seen in
// the wild differ between firmware generations, so we walk the buffer
// tolerantly looking for [statusId][size][value] or [statusId][value].

static void applyStatus(uint8_t id, uint32_t value) {
    switch (id) {
        case GP_STATUS_ENCODING:
            if (value == 1 && _telemetry.state != CAM_STATE_RECORDING) {
                _recStartMs = millis();   // Start local rec-time clock
            }
            _telemetry.state =
                value ? CAM_STATE_RECORDING : CAM_STATE_STANDBY;
            _telemetry.dataValid = true;
            break;

        case GP_STATUS_BATTERY_PCT:
            if (value <= 100) _telemetry.batteryPercent = value;
            _telemetry.dataValid = true;
            break;

        case GP_STATUS_BATTERY_BARS:
            if (_telemetry.batteryPercent > 100) {
                // Only use bars if percentage isn't available.
                static const uint8_t barsToPct[5] = {5, 25, 50, 75, 95};
                if (value < 5) _telemetry.batteryPercent = barsToPct[value];
            }
            break;

        default:
            break;
    }
}

// Notifications arrive as [header][len?][entries…], entries are pairs of
// [statusId][uint8 value] for the statuses we register.  Header bytes seen in
// the wild: 0xB3 (async update), 0x93/0x53/0x13/0xD3 (query responses).
static void parseQueryResponse(const uint8_t *data, size_t len) {
    size_t i = 0;

    // Skip leading header/length bytes.
    while (i + 1 < len) {
        uint8_t b = data[i];
        if (b == 0xB3 || b == 0x93 || b == 0xD3 || b == 0x53 || b == 0x13) {
            i++;
            continue;
        }
        break;
    }
    if (i < len && (len - i - 1) == data[i]) {
        i++;  // Looks like an explicit length byte — skip it too.
    }

    // Walk [id][u8 value] pairs.
    while (i + 1 < len) {
        uint8_t id = data[i];
        uint8_t val = data[i + 1];
        i += 2;
        applyStatus(id, val);
        DBG("GP: status 0x%02X = %u", id, val);
    }
}

static void notifyCallback(NimBLERemoteCharacteristic *pChar, uint8_t *pData,
                           size_t length, bool isNotify) {
    if (!length) return;

    if (pChar == _pQueryChar || pData[0] == 0xB3 || pData[0] == 0x93) {
        parseQueryResponse(pData, length);
        return;
    }

    // Command responses (0x83 frames): treat any response as link activity.
    if (pData[0] == 0x83) {
        DBG("GP: command ack (%d bytes)", length);
        return;
    }

    char hexBuf[128] = "";
    size_t hexLen = 0;
    for (size_t i = 0; i < length && i < 32; i++) {
        hexLen += snprintf(hexBuf + hexLen, sizeof(hexBuf) - hexLen, "%02X ", pData[i]);
    }
    DBG("GP: <<< notify (%d bytes): %s", length, hexBuf);
}

// ──────────────────────────────────────────────────────────────────────────────
// Public API
// ──────────────────────────────────────────────────────────────────────────────

void gpInit() {
    DBG("GP: Backend ready");
    _bleState = BLE_DISCONNECTED;
    _telemetry = CameraTelemetry();
    _registeredStatuses = false;
}

void gpUpdate() {
    uint32_t now = millis();

    switch (_bleState) {
        case BLE_DISCONNECTED:
            // Honour pending direct-connect (Web UI "Use" / camKick) at once
            // instead of stalling until the next scan interval.
            if (_doConnect) {
                connectToCamera();
                _doConnect = false;
                break;
            }
            if (now - _lastReconnectAttempt >= BLE_RECONNECT_INTERVAL_MS) {
                _lastReconnectAttempt = now;
                startScan();
            }
            break;

        case BLE_SCANNING:
            if (_doConnect) {
                connectToCamera();
                _doConnect = false;
                break;
            }
            // The 5-second window closes automatically (scanCompleteCb fires).
            // If the scan finished, restart it to keep discovery continuous
            // while still giving 60% airtime to Wi-Fi.
            if (!NimBLEDevice::getScan()->isScanning()) {
                _lastReconnectAttempt = now;
                startScan();
            }
            break;

        case BLE_AUTHENTICATING:
        case BLE_CONNECTED: {
            if (!_pClient || !_pClient->isConnected()) {
                _bleState = BLE_DISCONNECTED;
                break;
            }

            // Local rec-time clock while the camera reports encoding.
            if (_telemetry.state == CAM_STATE_RECORDING && _recStartMs != 0) {
                _telemetry.recTimeSeconds = (now - _recStartMs) / 1000;
            }

            // Keep-alive every ~3 s (Open GoPro best practice).
            if (now - _lastKeepAlive >= GOPRO_KEEPALIVE_INTERVAL_MS) {
                _lastKeepAlive = now;
                sendKeepAlive();
            }

            // Re-register + poll statuses shortly after connecting (the first
            // registration can be lost while the pairing prompt is pending).
            if (now - _connectedAtMs >= 4000 && !_registeredStatuses) {
                sendRegisterStatusUpdates();
            }
            if (now - _lastStatusPoll >= GOPRO_STATUS_POLL_MS) {
                _lastStatusPoll = now;
                sendStatusQuery();
            }
            break;
        }
    }
}

bool gpSendStartRecord() {
    if (_bleState != BLE_CONNECTED) return false;
    sendShutter(true);
    return true;
}

bool gpSendStopRecord() {
    if (_bleState != BLE_CONNECTED) return false;
    sendShutter(false);
    return true;
}

BleConnectionState gpGetState() { return _bleState; }
const CameraTelemetry& gpGetTelemetry() { return _telemetry; }
bool gpIsReady() { return (_bleState == BLE_CONNECTED); }

void gpTargetMac(const char *mac) {
    if (!mac || !*mac) return;
    NimBLEScan *pScan = NimBLEDevice::getScan();
    if (pScan && pScan->isScanning()) pScan->stop();
    if (_pClient && _pClient->isConnected()) _pClient->disconnect();
    _targetAddress    = NimBLEAddress(mac);
    _hasTargetAddress = true;
    _doConnect        = true;
    DBG("GP: targeting %s (direct connect)", mac);
}
