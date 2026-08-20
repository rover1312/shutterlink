// ============================================================================
// main.cpp — ESP32-C3 ShutterLink: Main Entry Point & State Machine
// ============================================================================
// Open-source camera control bridge.
//
// This firmware bridges a Betaflight Flight Controller (MSP / UART) with a
// DJI Action 2 camera (NimBLE / BLE).  It reads an RC switch position from
// the FC, triggers Start/Stop recording on the camera, and injects battery
// and recording-time telemetry back into the FC's HD OSD.
//
// ─── Main Loop Architecture (non-blocking) ──────────────────────────────────
//
//   ┌──────────────────────────────────────────────────────────┐
//   │                      loop()                             │
//   │                                                          │
//   │  1. bleUpdate()          — BLE scan/connect/keep-alive   │
//   │  2. mspPollRC()          — Send MSP_RC request on timer  │
//   │  3. mspReadIncoming()    — Parse UART bytes              │
//   │  4. processRcSwitch()    — Debounced toggle detection    │
//   │  5. updateOSD()          — Push telemetry text on timer  │
//   │  6. updateStatusLED()    — Blink pattern for feedback    │
//   │                                                          │
//   └──────────────────────────────────────────────────────────┘
//
// ============================================================================

#include <Arduino.h>
#include "config.h"
#include "msp_protocol.h"
#include "dji_ble_client.h"

// ──────────────────────────────────────────────────────────────────────────────
// Internal State
// ──────────────────────────────────────────────────────────────────────────────

// RC switch tracking
static bool     _switchIsOn          = false;   // Current debounced state
static bool     _switchRawIsOn       = false;   // Raw (un-debounced) state
static uint32_t _switchLastChange    = 0;       // millis() of last raw change
static uint16_t _lastRcValue         = 1000;    // Last read RC channel value (µs)
static bool     _rcDataValid         = false;   // True after first valid MSP_RC response

// Timers
static uint32_t _lastMspPoll         = 0;       // Last MSP_RC request time
static uint32_t _lastOsdUpdate       = 0;       // Last OSD text push time

// LED blink state
static uint32_t _lastLedToggle       = 0;
static bool     _ledState            = false;

// ──────────────────────────────────────────────────────────────────────────────
// LED Helpers
// ──────────────────────────────────────────────────────────────────────────────

static void ledOn() {
    digitalWrite(STATUS_LED_PIN, LED_ACTIVE_LOW ? LOW : HIGH);
    _ledState = true;
}

static void ledOff() {
    digitalWrite(STATUS_LED_PIN, LED_ACTIVE_LOW ? HIGH : LOW);
    _ledState = false;
}

/// Blink the LED at a given interval (non-blocking).
static void ledBlink(uint32_t intervalMs) {
    uint32_t now = millis();
    if (now - _lastLedToggle >= intervalMs) {
        _lastLedToggle = now;
        if (_ledState) ledOff(); else ledOn();
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// Status LED Patterns
// ──────────────────────────────────────────────────────────────────────────────
//   • BLE disconnected / scanning : slow blink (1000 ms)
//   • BLE connecting              : fast blink (200 ms)
//   • BLE connected, standby      : solid ON
//   • BLE connected, recording    : medium blink (500 ms)

static void updateStatusLED() {
    BleConnectionState bleState = bleGetState();

    switch (bleState) {
        case BLE_DISCONNECTED:
        case BLE_SCANNING:
            ledBlink(1000);
            break;

        case BLE_CONNECTING:
        case BLE_AUTHENTICATING:
            ledBlink(200);
            break;

        case BLE_CONNECTED: {
            const CameraTelemetry &tel = bleGetTelemetry();
            if (tel.state == CAM_STATE_RECORDING) {
                ledBlink(500);  // Recording: medium blink
            } else {
                ledOn();        // Standby: solid
            }
            break;
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// MSP — Periodic RC channel polling
// ──────────────────────────────────────────────────────────────────────────────

static void mspPollRC() {
    uint32_t now = millis();
    if (now - _lastMspPoll >= MSP_RC_POLL_INTERVAL_MS) {
        _lastMspPoll = now;
        mspSendRequest(MSP_RC);
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// MSP — Read and parse incoming UART bytes
// ──────────────────────────────────────────────────────────────────────────────

static void mspReadIncoming() {
    MspMessage msg;

    // Drain the UART buffer byte-by-byte into the MSP parser.
    while (Serial1.available()) {
        uint8_t byte = Serial1.read();

        if (mspParseByte(byte, msg)) {
            // ── Complete MSP message received ───────────────────────────
            if (msg.cmd == MSP_RC && msg.valid && !msg.isError) {
                // Extract the configured AUX channel value.
                _lastRcValue = mspGetRcChannel(msg, AUX_CHANNEL_INDEX);
                _rcDataValid = true;

                // Determine raw switch state (before debounce).
                bool rawOn = (_lastRcValue > RC_SWITCH_THRESHOLD);
                if (rawOn != _switchRawIsOn) {
                    _switchRawIsOn    = rawOn;
                    _switchLastChange = millis();
                }
            }
            // (Other MSP responses can be handled here if needed.)
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// RC Switch — Debounced state transitions
// ──────────────────────────────────────────────────────────────────────────────

static void processRcSwitch() {
    if (!_rcDataValid) return;  // No RC data yet — nothing to do.

    uint32_t now = millis();

    // Only accept a state change after the debounce interval.
    if (_switchRawIsOn != _switchIsOn &&
        (now - _switchLastChange >= RC_DEBOUNCE_MS)) {

        bool wasOn = _switchIsOn;
        _switchIsOn = _switchRawIsOn;

        // ── Edge detection ──────────────────────────────────────────────
        if (_switchIsOn && !wasOn) {
            // ── OFF → ON transition: Start Recording ────────────────────
            DBG("SWITCH: Record ON (ch%d = %d µs)",
                AUX_CHANNEL_INDEX, _lastRcValue);

            if (bleIsReady()) {
                if (bleSendStartRecord()) {
                    DBG("ACTION: Start Record command sent");
                } else {
                    DBG("ACTION: Failed to send Start Record");
                }
            } else {
                DBG("ACTION: Camera not ready — cannot start recording");
            }
        }
        else if (!_switchIsOn && wasOn) {
            // ── ON → OFF transition: Stop Recording ─────────────────────
            DBG("SWITCH: Record OFF (ch%d = %d µs)",
                AUX_CHANNEL_INDEX, _lastRcValue);

            if (bleIsReady()) {
                if (bleSendStopRecord()) {
                    DBG("ACTION: Stop Record command sent");
                } else {
                    DBG("ACTION: Failed to send Stop Record");
                }
            } else {
                DBG("ACTION: Camera not ready — cannot stop recording");
            }
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// OSD — Periodic text update
// ──────────────────────────────────────────────────────────────────────────────

static void updateOSD() {
    uint32_t now = millis();
    if (now - _lastOsdUpdate < OSD_UPDATE_INTERVAL_MS) return;
    _lastOsdUpdate = now;

    // Build the OSD string from BLE telemetry.
    char osdBuf[OSD_MAX_TEXT_LEN + 1];
    bleFormatOSDString(osdBuf, sizeof(osdBuf));

    // Send to the FC via MSP v2 SET_TEXT.
    mspSendOSDText(OSD_TEXT_TYPE, OSD_TEXT_ROW, OSD_TEXT_COL, osdBuf);
}

// ──────────────────────────────────────────────────────────────────────────────
// Arduino setup()
// ──────────────────────────────────────────────────────────────────────────────

void setup() {
    // ── USB Serial for debug output ─────────────────────────────────────
    Serial.begin(115200);
    delay(1000);  // Allow USB CDC to enumerate

    DBG("============================================");
    DBG("  ESP32-C3 ShutterLink");
    DBG("  Firmware v1.0.0");
    DBG("============================================");

    // ── Status LED ──────────────────────────────────────────────────────
    pinMode(STATUS_LED_PIN, OUTPUT);
    ledOff();

    // ── MSP / UART ──────────────────────────────────────────────────────
    mspInit(Serial1);
    DBG("MSP: Initialised");

    // ── BLE ─────────────────────────────────────────────────────────────
    bleInit();
    DBG("BLE: Initialised");

    DBG("SETUP: Complete — entering main loop");
    DBG("CONFIG: AUX channel = %d, threshold = %d µs, debounce = %d ms",
        AUX_CHANNEL_INDEX, RC_SWITCH_THRESHOLD, RC_DEBOUNCE_MS);
}

// ──────────────────────────────────────────────────────────────────────────────
// Arduino loop() — Non-blocking state machine
// ──────────────────────────────────────────────────────────────────────────────

void loop() {
    // 1. BLE management: scan, connect, keep-alive, auto-reconnect.
    bleUpdate();

    // 2. Send periodic MSP_RC requests to the Flight Controller.
    mspPollRC();

    // 3. Parse any incoming UART bytes from the FC.
    mspReadIncoming();

    // 4. Process debounced RC switch transitions → trigger BLE commands.
    processRcSwitch();

    // 5. Push OSD telemetry text to the FC every second.
    updateOSD();

    // 6. Update status LED blink pattern.
    updateStatusLED();

        // ── DEBUG HEARTBEAT ─────────────────────────────────────────────
    static uint32_t lastStatePrint = 0;
    if (millis() - lastStatePrint > 5000) {
        lastStatePrint = millis();
        // 0=Disc, 1=Scan, 2=Conn, 3=Auth, 4=Ready
        DBG("STATE: BLE=%d, RC=%d, Switch=%s", 
            bleGetState(), _lastRcValue, _switchIsOn ? "ON" : "OFF");
    }

    // Yield to the RTOS scheduler (good practice on ESP32).
    yield();
}
