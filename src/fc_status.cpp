// ============================================================================
// fc_status.cpp — Flight Controller state polling via MSP
// ============================================================================
//
// Arming detection:
//   MSP_BOXIDS returns the list of active "box" IDs.  Betaflight's ARM box
//   has ID 0; its *position* within the BOXIDS array is the bit index used
//   in MSP_STATUS.flightModeFlags.  This mirrors how OSD devices detect ARM.
// ============================================================================

#include "fc_status.h"

// ──────────────────────────────────────────────────────────────────────────────
// Internal state
// ──────────────────────────────────────────────────────────────────────────────

static FcTelemetry _fc = {false, false, 0, 0, 0};

static uint32_t _lastStatusPoll    = 0;
static uint32_t _lastAnalogPoll    = 0;
static uint32_t _lastBoxIdsPoll    = 0;
static uint32_t _lastMspRxTime     = 0;
static int8_t   _armBitIndex       = -1;     // -1 = unknown yet

// One-shot identity fetch state machine
static bool     _identityDone      = false;
static uint32_t _lastIdentityReq   = 0;
static uint8_t  _identityStep      = 0;

static char _apiVersion[10]  = {0};
static char _variant[6]      = {0};
static char _fwVersion[12]   = {0};
static char _boardName[17]   = {0};

// ──────────────────────────────────────────────────────────────────────────────
// Public API
// ──────────────────────────────────────────────────────────────────────────────

void fcStatusInit() {
    _fc.armed   = false;
    _fc.fcAlive = false;
}

const FcTelemetry& fcGetTelemetry() { return _fc; }
const char* fcApiVersion()      { return _apiVersion; }
const char* fcVariant()         { return _variant; }
const char* fcFirmwareVersion() { return _fwVersion; }
const char* fcBoardName()       { return _boardName; }

// ──────────────────────────────────────────────────────────────────────────────
// Feed parsed MSP responses
// ──────────────────────────────────────────────────────────────────────────────

void fcStatusFeed(const MspMessage &msg) {
    if (!msg.valid || msg.isError) return;
    _lastMspRxTime = millis();

    switch (msg.cmd) {

        case MSP_STATUS: {
            // cycleTime u16 | i2cErrors u16 | sensors u16 | flags u32 | profile u8
            if (msg.payloadSize < 11) break;
            _fc.cycleTimeUs = mspReadU16(msg, 0);
            uint32_t flags  = mspReadU32(msg, 6);

            if (_armBitIndex >= 0) {
                _fc.armed = (flags >> _armBitIndex) & 0x01;
            } else {
                // BOXIDS not received yet — assume the common default (bit 0).
                _fc.armed = flags & 0x01;
            }
            break;
        }

        case MSP_ANALOG: {
            // vbat u8 (0.1 V units) | mAhDrawn u16 | rssi u16 | amperage i16
            if (msg.payloadSize < 5) break;
            _fc.vbat10 = msg.payload[0];
            _fc.rssi   = mspReadU16(msg, 3);
            break;
        }

        case MSP_BOXIDS: {
            // Array of active box IDs. Find position of box 0 (= ARM).
            for (uint8_t i = 0; i < msg.payloadSize; i++) {
                if (msg.payload[i] == 0) {
                    if (_armBitIndex != (int8_t)i) {
                        DBG("FC: ARM bit index = %d", i);
                    }
                    _armBitIndex = (int8_t)i;
                    break;
                }
            }
            break;
        }

        case MSP_API_VERSION: {
            if (msg.payloadSize < 3 || _identityDone) break;
            snprintf(_apiVersion, sizeof(_apiVersion), "%u.%u",
                     msg.payload[1], msg.payload[2]);
            DBG("FC: API version %s", _apiVersion);
            break;
        }

        case MSP_FC_VARIANT: {
            if (msg.payloadSize < 4) break;
            memcpy(_variant, msg.payload, 4);
            _variant[4] = '\0';
            DBG("FC: variant %s", _variant);
            break;
        }

        case MSP_FC_VERSION: {
            if (msg.payloadSize < 3) break;
            snprintf(_fwVersion, sizeof(_fwVersion), "%u.%u.%u",
                     msg.payload[0], msg.payload[1], msg.payload[2]);
            DBG("FC: firmware %s", _fwVersion);
            break;
        }

        case MSP_BOARD_INFO: {
            if (msg.payloadSize < 4) break;
            memcpy(_boardName, msg.payload, 4);
            _boardName[4] = '\0';
            // Newer API versions append: revision u16, capabilities u8,
            // then a length-prefixed target name.
            if (msg.payloadSize > 8) {
                uint8_t len = msg.payload[7];
                if (len > sizeof(_boardName) - 1) len = sizeof(_boardName) - 1;
                if (len && msg.payloadSize >= (uint8_t)(8 + len)) {
                    memcpy(_boardName, &msg.payload[8], len);
                    _boardName[len] = '\0';
                }
            }
            DBG("FC: board %s", _boardName);
            break;
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// Periodic polling — call from loop()
// ──────────────────────────────────────────────────────────────────────────────

void fcStatusUpdate() {
    uint32_t now = millis();

    _fc.fcAlive = _fc.fcAlive || (now - _lastMspRxTime <= 2000 && _lastMspRxTime != 0);
    if (_lastMspRxTime != 0) {
        _fc.fcAlive = (now - _lastMspRxTime) <= 2000;
    }

    // Staggered identity queries during the first seconds after boot.
    if (!_identityDone) {
        static const uint8_t kIdentityCmds[] = {
            MSP_API_VERSION, MSP_FC_VARIANT, MSP_FC_VERSION, MSP_BOARD_INFO
        };
        if (now - _lastIdentityReq >= 300) {
            _lastIdentityReq = now;
            mspSendRequest(kIdentityCmds[_identityStep]);
            if (++_identityStep >= sizeof(kIdentityCmds)) {
                _identityDone = true;
                // Kick off regular polls immediately afterwards.
                _lastStatusPoll = _lastAnalogPoll = _lastBoxIdsPoll = 0;
            }
        }
        return;  // Don't interleave regular polls with identity discovery
    }

    if (now - _lastStatusPoll >= MSP_STATUS_POLL_INTERVAL_MS) {
        _lastStatusPoll = now;
        mspSendRequest(MSP_STATUS);
    }

    if (now - _lastBoxIdsPoll >= MSP_BOXIDS_POLL_INTERVAL_MS) {
        _lastBoxIdsPoll = now;
        mspSendRequest(MSP_BOXIDS);
    }

    if (now - _lastAnalogPoll >= MSP_ANALOG_POLL_INTERVAL_MS) {
        _lastAnalogPoll = now;
        mspSendRequest(MSP_ANALOG);
    }
}
