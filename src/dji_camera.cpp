// ============================================================================
// dji_camera.cpp — NimBLE BLE Central for DJI Osmo Action (DUML protocol)
// ============================================================================
// Implements the exact DUML-over-BLE protocol verified by lib-osmo-ble and
// osmosis.  Field-tested: pairing + record start/stop.
// ============================================================================

#include "dji_camera.h"
#include "cam_registry.h"
#include <NimBLEDevice.h>

// ──────────────────────────────────────────────────────────────────────────────
// DJI GATT UUIDs
// ──────────────────────────────────────────────────────────────────────────────
static NimBLEUUID DJI_SERVICE_UUID("0000fff0-0000-1000-8000-00805f9b34fb");
static NimBLEUUID DJI_CHAR_FFF3("0000fff3-0000-1000-8000-00805f9b34fb");
static NimBLEUUID DJI_CHAR_FFF4("0000fff4-0000-1000-8000-00805f9b34fb"); // Pairing Arm + Notifications
static NimBLEUUID DJI_CHAR_FFF5("0000fff5-0000-1000-8000-00805f9b34fb"); // DUML Commands (WriteNoResponse)

// ──────────────────────────────────────────────────────────────────────────────
// Internal state
// ──────────────────────────────────────────────────────────────────────────────
static BleConnectionState  _bleState         = BLE_DISCONNECTED;
static CameraTelemetry     _telemetry;

static NimBLEClient             *_pClient    = nullptr;
static NimBLERemoteCharacteristic *_pControlChar   = nullptr; // fff5
static NimBLERemoteCharacteristic *_pTelemetryChar = nullptr; // fff3
static NimBLERemoteCharacteristic *_pAuthChar      = nullptr; // fff4

static NimBLEAddress          _targetAddress;
static bool                   _hasTargetAddress = false;
static bool                   _doConnect     = false;

static bool                   _sessionEstablished = false;
static uint16_t               _sequenceCounter    = 1;

static uint32_t _lastReconnectAttempt = 0;
static uint32_t _lastKeepAlive        = 0;
static uint32_t _pairingArmedTime     = 0;

// ──────────────────────────────────────────────────────────────────────────────
// Forward declarations
// ──────────────────────────────────────────────────────────────────────────────
static void    startScan();
static bool    connectToCamera();
static void    notifyCallback(NimBLERemoteCharacteristic *pChar, uint8_t *pData,
                              size_t length, bool isNotify);
static bool    isDjiDevice(NimBLEAdvertisedDevice *device);
static void    sendPairingArm();
static void    sendSetPairingPIN();
static void    sendKeepAlive();
static size_t  buildDumlPacket(uint8_t *buffer, uint8_t sender, uint8_t receiver,
                               uint16_t msgId, uint8_t flags, uint8_t cmdSet, uint8_t cmdId,
                               const uint8_t *payload, size_t payloadLen);
static uint8_t  crc8_dji(const uint8_t *data, size_t len);
static uint16_t crc16_dji(const uint8_t *data, size_t len);

// ──────────────────────────────────────────────────────────────────────────────
// BLE Callbacks
// ──────────────────────────────────────────────────────────────────────────────
class ShutterLinkClientCallbacks : public NimBLEClientCallbacks {
    void onConnect(NimBLEClient *pClient) override {
        DBG("DJI: Connected to camera");
    }
    void onDisconnect(NimBLEClient *pClient) {
        DBG("DJI: Disconnected from camera");
        _bleState = BLE_DISCONNECTED;
        _sessionEstablished = false;
        _pControlChar   = nullptr;
        _pTelemetryChar = nullptr;
        _pAuthChar      = nullptr;
        _telemetry.dataValid = false;
        _telemetry.state     = CAM_STATE_UNKNOWN;
    }
};
static ShutterLinkClientCallbacks _clientCallbacks;

class ShutterLinkAdvertisedDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks {
    void onResult(NimBLEAdvertisedDevice *advertisedDevice) override {
        if (isDjiDevice(advertisedDevice)) {
            std::string macStr = advertisedDevice->getAddress().toString();
            std::string nameStr = advertisedDevice->getName();

            // Learning is always allowed; CONNECTING is only permitted for
            // the saved camera currently marked active (user-approved).
            bool mayAuto = camRegistryMayAutoConnect(CAMERA_DJI, macStr.c_str());
            camRegistryRemember(CAMERA_DJI, macStr.c_str(), nameStr.c_str());

            DBG("DJI: seen %s (%s)%s", nameStr.c_str(), macStr.c_str(),
                mayAuto ? "" : "  [not active — ignoring]");

            if (!mayAuto) return;   // Keep scanning, never pair uninvited

            DBG("DJI: Found active camera: %s (%s) RSSI=%d",
                nameStr.c_str(), macStr.c_str(), advertisedDevice->getRSSI());

            NimBLEDevice::getScan()->stop();
            _targetAddress     = advertisedDevice->getAddress();
            _hasTargetAddress  = true;
            _doConnect         = true;
        }
    }
};
static ShutterLinkAdvertisedDeviceCallbacks _scanCallbacks;

// ──────────────────────────────────────────────────────────────────────────────
// Device identification
// ──────────────────────────────────────────────────────────────────────────────
static const char *DJI_NAME_PREFIXES[] = {
    "Osmo Action", "DJI Action", "OSMO ACTION", "DJI ACTION",
    "Action 2", "action2", "Action 4", "Action 5", "OsmoAction",
};
static const size_t DJI_NAME_PREFIX_COUNT = sizeof(DJI_NAME_PREFIXES) / sizeof(DJI_NAME_PREFIXES[0]);

static bool isDjiDevice(NimBLEAdvertisedDevice *device) {
    if (device->haveName()) {
        std::string name = device->getName();
        for (size_t i = 0; i < DJI_NAME_PREFIX_COUNT; i++) {
            if (name.find(DJI_NAME_PREFIXES[i]) != std::string::npos) return true;
        }
    }
    std::string addr = device->getAddress().toString();
    if (addr.rfind("34:d2:62", 0) == 0 || addr.rfind("60:60:1f", 0) == 0) {
        return true;
    }
    return false;
}

// ──────────────────────────────────────────────────────────────────────────────
// Scan management
// ──────────────────────────────────────────────────────────────────────────────

/// No-op completion handler — required so NimBLE uses the *non-blocking*
/// scan API.  The two-argument start(duration, bool) overload BLOCKS the
/// calling task until the scan finishes (forever when duration == 0),
/// which would freeze the entire main loop.
static void scanCompleteCb(NimBLEScanResults results) {
    DBG("DJI: Scan window complete (%d devices)", results.getCount());
}

static void startScan() {
    if (_bleState == BLE_SCANNING) return;
    DBG("DJI: Starting continuous scan for camera...");
    _bleState     = BLE_SCANNING;
    _doConnect    = false;

    NimBLEScan *pScan = NimBLEDevice::getScan();
    pScan->setAdvertisedDeviceCallbacks(&_scanCallbacks, false);
    pScan->setActiveScan(true);
    pScan->setInterval(100);
    pScan->setWindow(99);
    pScan->setMaxResults(0);
    // Non-blocking overload (callback variant) — duration 0 = scan forever.
    pScan->start(0, scanCompleteCb);
}

// ──────────────────────────────────────────────────────────────────────────────
// Connection and GATT Discovery
// ──────────────────────────────────────────────────────────────────────────────
static bool connectToCamera() {
    if (!_hasTargetAddress) return false;
    DBG("DJI: Connecting to target %s...", _targetAddress.toString().c_str());
    _bleState = BLE_CONNECTING;

    if (!_pClient) {
        _pClient = NimBLEDevice::createClient();
        _pClient->setClientCallbacks(&_clientCallbacks, false);
        _pClient->setConnectTimeout(BLE_CONNECT_TIMEOUT_MS / 1000);
    }

    if (!_pClient->connect(_targetAddress)) {
        DBG("DJI: Connection failed!");
        _bleState = BLE_DISCONNECTED;
        return false;
    }

    DBG("DJI: Connected! Discovering GATT services...");
    NimBLERemoteService* pService = _pClient->getService(DJI_SERVICE_UUID);
    if (!pService) {
        DBG("DJI: Service 0xFFF0 not found! Disconnecting.");
        _pClient->disconnect();
        _bleState = BLE_DISCONNECTED;
        return false;
    }

    _pTelemetryChar = pService->getCharacteristic(DJI_CHAR_FFF3);
    _pAuthChar      = pService->getCharacteristic(DJI_CHAR_FFF4);
    _pControlChar   = pService->getCharacteristic(DJI_CHAR_FFF5);

    if (!_pControlChar || !_pAuthChar) {
        DBG("DJI: Missing FFF4 or FFF5 characteristics!");
        _pClient->disconnect();
        _bleState = BLE_DISCONNECTED;
        return false;
    }

    // Subscribe to notifications on FFF4 (pairing/telemetry) and FFF5
    if (_pAuthChar && _pAuthChar->canNotify()) {
        _pAuthChar->subscribe(true, notifyCallback);
        DBG("DJI: Subscribed to FFF4 notifications");
    }
    if (_pControlChar && _pControlChar->canNotify()) {
        _pControlChar->subscribe(true, notifyCallback);
        DBG("DJI: Subscribed to FFF5 notifications");
    }

    strlcpy(_telemetry.model, "DJI Osmo Action", sizeof(_telemetry.model));

    // Start pairing sequence
    _bleState = BLE_AUTHENTICATING;
    _sessionEstablished = false;
    sendPairingArm();
    return true;
}

// ──────────────────────────────────────────────────────────────────────────────
// DJI Protocol Implementation (DUML)
// ──────────────────────────────────────────────────────────────────────────────
static uint8_t crc8_dji(const uint8_t *data, size_t len) {
    uint8_t crc = 0x77; // reflected init
    uint8_t poly = 0x8C; // reflected poly
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x01) crc = (crc >> 1) ^ poly;
            else crc >>= 1;
        }
    }
    return crc;
}

static uint16_t crc16_dji(const uint8_t *data, size_t len) {
    uint16_t crc = 0x3692; // reflected init
    uint16_t poly = 0x8408; // reflected poly
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x0001) crc = (crc >> 1) ^ poly;
            else crc >>= 1;
        }
    }
    return crc;
}

static size_t buildDumlPacket(uint8_t *buffer, uint8_t sender, uint8_t receiver,
                              uint16_t msgId, uint8_t flags, uint8_t cmdSet, uint8_t cmdId,
                              const uint8_t *payload, size_t payloadLen) {
    size_t idx = 0;
    uint16_t totalLen = 13 + payloadLen;

    buffer[idx++] = 0x55;
    buffer[idx++] = totalLen & 0xFF;
    buffer[idx++] = 0x04 | ((totalLen >> 8) & 0x03); // ver=1
    buffer[idx++] = crc8_dji(buffer, 3);

    // Target (LE)
    buffer[idx++] = sender;
    buffer[idx++] = receiver;

    // Msg ID (BE)
    buffer[idx++] = (msgId >> 8) & 0xFF;
    buffer[idx++] = msgId & 0xFF;

    // Type
    buffer[idx++] = flags;
    buffer[idx++] = cmdSet;
    buffer[idx++] = cmdId;

    // Payload
    for (size_t i = 0; i < payloadLen; i++) {
        buffer[idx++] = payload[i];
    }

    // CRC16 (LE)
    uint16_t crc = crc16_dji(buffer, idx);
    buffer[idx++] = crc & 0xFF;
    buffer[idx++] = (crc >> 8) & 0xFF;

    return idx;
}

static void sendPairingArm() {
    // Step 1: Write [0x01, 0x00] to fff4 to arm pairing mode
    uint8_t armPairing[] = {0x01, 0x00};
    if (_pAuthChar && _pAuthChar->canWrite()) {
        _pAuthChar->writeValue(armPairing, 2, true); // Write with response
        DBG("DJI: Wrote [0x01, 0x00] to FFF4 to arm pairing");
        _pairingArmedTime = millis();
    }
}

static void sendSetPairingPIN() {
    uint8_t packet[64];

    // Payload: PackString("001749319286102") + PackString("osmo")
    uint8_t payload[32];
    size_t pIdx = 0;

    const char* id = "001749319286102";
    payload[pIdx++] = strlen(id);
    memcpy(&payload[pIdx], id, strlen(id));
    pIdx += strlen(id);

    const char* pin = "osmo";
    payload[pIdx++] = strlen(pin);
    memcpy(&payload[pIdx], pin, strlen(pin));
    pIdx += strlen(pin);

    // Target: App (0x02) -> WiFi (0x07). Type: flags=0x40, set=0x07, id=0x45
    size_t len = buildDumlPacket(packet, 0x02, 0x07, _sequenceCounter++,
                                 0x40, 0x07, 0x45, payload, pIdx);

    // MUST use Write-Without-Response (false) on FFF5
    if (_pControlChar && _pControlChar->canWriteNoResponse()) {
        _pControlChar->writeValue(packet, len, false);
        DBG("DJI: Sent SetPairingPIN DUML packet to FFF5 (%d bytes)", len);
    }
}

static void sendKeepAlive() {
    // Send a generic ping to keep the link alive
    uint8_t packet[32];
    uint8_t payload[] = {0x00};
    size_t len = buildDumlPacket(packet, 0x02, 0x01, _sequenceCounter++,
                                 0x40, 0x00, 0xF1, payload, 1);
    if (_pControlChar && _pControlChar->canWriteNoResponse()) {
        _pControlChar->writeValue(packet, len, false);
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// Notification Callback
// ──────────────────────────────────────────────────────────────────────────────
static void notifyCallback(NimBLERemoteCharacteristic *pChar, uint8_t *pData,
                           size_t length, bool isNotify) {
    const char *charName = (pChar == _pAuthChar) ? "FFF4" : "FFF5";

    if (length < 13 || pData[0] != 0x55) {
        char hexBuf[128] = "";
        size_t hexLen = 0;
        for (size_t i = 0; i < length && i < 32; i++) {
            hexLen += snprintf(hexBuf + hexLen, sizeof(hexBuf) - hexLen, "%02X ", pData[i]);
        }
        DBG("DJI: <<< NON-DUML [%s] (%d bytes): %s", charName, length, hexBuf);
        return;
    }

    uint8_t flags = pData[8];
    uint8_t cmdSet = pData[9];
    uint8_t cmdId = pData[10];

    DBG("DJI: <<< DUML [%s] flags=0x%02X set=0x%02X id=0x%02X len=%d",
        charName, flags, cmdSet, cmdId, length);

    // Check for Pairing Status Response (flags=0xC0, set=0x07, id=0x45)
    if (flags == 0xC0 && cmdSet == 0x07 && cmdId == 0x45) {
        if (length >= 12) {
            DBG("DJI: Pairing Status Payload: %02X %02X %02X", pData[11], pData[12], pData[13]);
            // payload[1] == 0x01 means Already Paired
            if (pData[12] == 0x01 || pData[11] == 0x01) {
                _sessionEstablished = true;
                _bleState = BLE_CONNECTED;
                DBG("DJI: *** ALREADY PAIRED - SESSION ESTABLISHED ***");
            }
            // payload[1] == 0x02 means Approval Required
            else if (pData[12] == 0x02 || pData[11] == 0x02) {
                DBG("DJI: Approval required - TAP APPROVE ON CAMERA SCREEN!");
            }
        }
    }

    // Check for Pairing Approved (flags=0x40, set=0x07, id=0x46)
    if (flags == 0x40 && cmdSet == 0x07 && cmdId == 0x46) {
        _sessionEstablished = true;
        _bleState = BLE_CONNECTED;
        DBG("DJI: *** PAIRING APPROVED - SESSION ESTABLISHED ***");

        // Send ACK back: flags=0xC0, set=0x07, id=0x46, payload=0x00
        uint8_t packet[16];
        uint8_t payload[] = {0x00};
        size_t len = buildDumlPacket(packet, 0x02, 0x07, _sequenceCounter++,
                                     0xC0, 0x07, 0x46, payload, 1);
        if (_pControlChar && _pControlChar->canWriteNoResponse()) {
            _pControlChar->writeValue(packet, len, false);
        }
    }

    // Unsolicited Telemetry (flags=0x00)
    if (flags == 0x00) {
        _telemetry.dataValid = true;
        // Basic parsing can be added here once we see the exact telemetry CmdSet/Id
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// Public API
// ──────────────────────────────────────────────────────────────────────────────
void djiInit() {
    DBG("DJI: Backend ready (NimBLE stack shared)");
    _bleState = BLE_DISCONNECTED;
    _telemetry = CameraTelemetry();
}

void djiUpdate() {
    uint32_t now = millis();

    switch (_bleState) {
        case BLE_DISCONNECTED:
            if (now - _lastReconnectAttempt >= BLE_RECONNECT_INTERVAL_MS) {
                _lastReconnectAttempt = now;
                startScan();
            }
            break;

        case BLE_SCANNING:
            if (_doConnect) {
                connectToCamera();
                _doConnect = false;
            }
            break;

        case BLE_AUTHENTICATING:
            // Wait 200ms after arming, then send the PIN packet
            if (now - _pairingArmedTime >= 200 && _pairingArmedTime > 0) {
                _pairingArmedTime = 0; // Only send once
                sendSetPairingPIN();
            }
            if (now - _lastReconnectAttempt >= 30000) {
                DBG("DJI: Auth timeout (30s) — resetting");
                if (_pClient) _pClient->disconnect();
                _bleState = BLE_DISCONNECTED;
                _lastReconnectAttempt = now;
            }
            break;

        case BLE_CONNECTED:
            if (!_pClient || !_pClient->isConnected()) {
                _bleState = BLE_DISCONNECTED;
                break;
            }
            if (now - _lastKeepAlive >= BLE_KEEPALIVE_INTERVAL_MS) {
                _lastKeepAlive = now;
                sendKeepAlive();
            }
            break;
    }
}

bool djiSendStartRecord() {
    if (_bleState != BLE_CONNECTED || !_sessionEstablished) return false;

    uint8_t packet[32];
    uint8_t payload[] = {0x01}; // 1 = Start

    // CmdSet 0x0A (Camera), CmdId 0x0D (Record)
    size_t len = buildDumlPacket(packet, 0x02, 0x01, _sequenceCounter++,
                                 0x40, 0x0A, 0x0D, payload, sizeof(payload));

    if (_pControlChar && _pControlChar->canWriteNoResponse()) {
        _pControlChar->writeValue(packet, len, false);
        DBG("DJI: Sent Start Record (0x0A/0x0D) to FFF5");
        return true;
    }
    return false;
}

bool djiSendStopRecord() {
    if (_bleState != BLE_CONNECTED || !_sessionEstablished) return false;

    uint8_t packet[32];
    uint8_t payload[] = {0x00}; // 0 = Stop

    size_t len = buildDumlPacket(packet, 0x02, 0x01, _sequenceCounter++,
                                 0x40, 0x0A, 0x0D, payload, sizeof(payload));

    if (_pControlChar && _pControlChar->canWriteNoResponse()) {
        _pControlChar->writeValue(packet, len, false);
        DBG("DJI: Sent Stop Record (0x0A/0x0D) to FFF5");
        return true;
    }
    return false;
}

BleConnectionState djiGetState() { return _bleState; }
const CameraTelemetry& djiGetTelemetry() { return _telemetry; }
bool djiIsReady() { return (_bleState == BLE_CONNECTED) && _sessionEstablished; }

void djiTargetMac(const char *mac) {
    if (!mac || !*mac) return;
    NimBLEScan *pScan = NimBLEDevice::getScan();
    if (pScan && pScan->isScanning()) pScan->stop();
    if (_pClient && _pClient->isConnected()) _pClient->disconnect();
    _targetAddress    = NimBLEAddress(mac);
    _hasTargetAddress = true;
    _doConnect        = true;
    DBG("DJI: targeting %s (direct connect)", mac);
}
