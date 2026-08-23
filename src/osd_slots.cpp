// ============================================================================
// osd_slots.cpp — Betaflight Custom Message 1..4 content manager
// ============================================================================
// Slot content options (OsdSlotContent in settings.h):
//   OFF / CAM_STATUS / REC_TIME / BATTERY / LINK / FC_BATT / ARM_STATE
//
// Betaflight limits each custom message to 16 characters (MAX_NAME_LENGTH),
// same as pilot/craft name.
// ============================================================================

#include "osd_slots.h"
#include "config.h"
#include "settings.h"
#include "camera_manager.h"
#include "fc_status.h"
#include "recorder.h"
#include "msp_protocol.h"

// ──────────────────────────────────────────────────────────────────────────────
// Internal state
// ──────────────────────────────────────────────────────────────────────────────

static uint32_t _lastPush        = 0;
static char     _lastSent[4][OSD_MAX_TEXT_LEN + 1] = {"", "", "", ""};
static bool     _firstRun        = true;

// ──────────────────────────────────────────────────────────────────────────────
// Content formatters
// ──────────────────────────────────────────────────────────────────────────────

static void formatMmSs(uint16_t seconds, char *out, size_t len) {
    snprintf(out, len, "%02u:%02u", seconds / 60, seconds % 60);
}

/// Build the string for one slot into `buf` (max OSD_MAX_TEXT_LEN chars).
static void buildSlotString(uint8_t content, char *buf, size_t bufLen) {
    const CameraTelemetry &tel = camGetTelemetry();
    const FcTelemetry &fc = fcGetTelemetry();

    switch (content) {

        case OSD_SLOT_CAM_STATUS: {
            if (camGetState() == BLE_DISCONNECTED ||
                camGetState() == BLE_SCANNING) {
                snprintf(buf, bufLen, "%s",
                         camGetState() == BLE_SCANNING ? "CAM SCAN" : "CAM OFF");
            } else if (!tel.dataValid && camGetState() != BLE_CONNECTED) {
                snprintf(buf, bufLen, "CAM PAIR");
            } else {
                uint16_t t = tel.recTimeSeconds;
                switch (tel.state) {
                    case CAM_STATE_RECORDING:
                        if (tel.batteryPercent <= 100) {
                            snprintf(buf, bufLen, "REC %u%% %02u:%02u",
                                     tel.batteryPercent, t / 60, t % 60);
                        } else {
                            formatMmSs(t, buf, bufLen);
                            snprintf(buf, bufLen, "REC %s", buf);
                        }
                        break;
                    case CAM_STATE_STANDBY:
                        if (tel.batteryPercent <= 100) {
                            snprintf(buf, bufLen, "STBY %u%%", tel.batteryPercent);
                        } else {
                            snprintf(buf, bufLen, "STBY");
                        }
                        break;
                    default:
                        snprintf(buf, bufLen, "CAM ???");
                        break;
                }
            }
            break;
        }

        case OSD_SLOT_REC_TIME: {
            if (!camIsReady()) { snprintf(buf, bufLen, "REC --:--"); break; }
            uint16_t t = tel.recTimeSeconds;
            snprintf(buf, bufLen, "REC %02u:%02u", t / 60, t % 60);
            break;
        }

        case OSD_SLOT_BATTERY: {
            if (tel.dataValid && tel.batteryPercent <= 100) {
                snprintf(buf, bufLen, "BAT %u%%", tel.batteryPercent);
            } else {
                snprintf(buf, bufLen, "BAT --");
            }
            break;
        }

        case OSD_SLOT_LINK: {
            switch (camGetState()) {
                case BLE_CONNECTED:     snprintf(buf, bufLen, "LINK READY"); break;
                case BLE_AUTHENTICATING:snprintf(buf, bufLen, "LINK PAIR"); break;
                case BLE_CONNECTING:    snprintf(buf, bufLen, "LINK CONN"); break;
                case BLE_SCANNING:      snprintf(buf, bufLen, "LINK SCAN"); break;
                default:                snprintf(buf, bufLen, "LINK OFF"); break;
            }
            break;
        }

        case OSD_SLOT_FC_BATT: {
            if (fc.vbat10 > 0) {
                snprintf(buf, bufLen, "FC %2u.%1uV", fc.vbat10 / 10, fc.vbat10 % 10);
            } else {
                snprintf(buf, bufLen, "FC --.-V");
            }
            break;
        }

        case OSD_SLOT_ARM_STATE: {
            if (!fc.fcAlive)     snprintf(buf, bufLen, "FC NOLINK");
            else if (fc.armed)   snprintf(buf, bufLen, "ARMED");
            else                 snprintf(buf, bufLen, "DISARMED");
            break;
        }

        case OSD_SLOT_OFF:
        default:
            buf[0] = '\0';
            break;
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// Public API
// ──────────────────────────────────────────────────────────────────────────────

void osdSlotsInit() {
    for (int i = 0; i < 4; i++) _lastSent[i][0] = '\0';
}

const char* osdSlotText(uint8_t slot) {
    if (slot >= 4) return "";
    return _lastSent[slot];
}

void osdSlotsUpdate() {
    uint32_t now = millis();
    if (!_firstRun && now - _lastPush < OSD_UPDATE_INTERVAL_MS) return;
    bool forcePush = _firstRun;
    _firstRun = false;
    _lastPush = now;

    const ShutterSettings &cfg = settingsGet();

    for (uint8_t slot = 0; slot < 4; slot++) {
        // Only push a slot when its text actually changed.
        char buf[OSD_MAX_TEXT_LEN + 1];
        buildSlotString(cfg.osdSlot[slot], buf, sizeof(buf));

        if (forcePush || strcmp(buf, _lastSent[slot]) != 0) {
            strlcpy(_lastSent[slot], buf, sizeof(_lastSent[slot]));
            mspSendSetText(MSP2TEXT_CUSTOM_MSG_0 + slot, buf);
        }
    }
}
