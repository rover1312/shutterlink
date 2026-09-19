// ============================================================================
// wifiswitch.cpp — Toggle the Web-UI Wi-Fi AP from a spare radio switch
// ============================================================================

#include "wifiswitch.h"
#include "config.h"
#include "settings.h"
#include "web_server.h"

// ──────────────────────────────────────────────────────────────────────────────
// Internal state
// ──────────────────────────────────────────────────────────────────────────────

static bool     _rcValid      = false;
static bool     _rawOn        = false;    // Un-debounced switch position
static bool     _stableOn     = true;     // Debounced position (boot default: on)
static uint32_t _lastRawChange = 0;      // 0 = no transition being qualified

// Minimum time a new raw position must hold before it is accepted. Uses a
// fixed 600 ms floor — Wi-Fi must never flap because of a bumpy thumb.
// (Deliberately independent from the record-switch debounce in settings:
// toggling a radio costs far more than a record edge.)
static const uint32_t kWifiSwitchDebounceMs = 600;

// ──────────────────────────────────────────────────────────────────────────────
// Public API
// ──────────────────────────────────────────────────────────────────────────────

void wifiSwitchInit() {
    _rcValid = false;
    _stableOn = true;   // AP starts enabled at boot (lock-out protection)
}

void wifiSwitchFeedRc(uint16_t rcValueUsec) {
    _rcValid = true;
    bool rawOn = (rcValueUsec > settingsGet().rcThresholdUs);
    if (rawOn != _rawOn) {
        // Raw edge — restart the qualification window from now.
        _rawOn = rawOn;
        _lastRawChange = millis();
    }
}

void wifiSwitchUpdate() {
    const ShutterSettings &cfg = settingsGet();
    if (cfg.wifiSwitchCh > 15) return;   // Feature disabled via Web UI

    if (!_rcValid) return;

    uint32_t now = millis();

    // If the raw position flipped back during the window, re-arm.
    if (_rawOn == _stableOn) {
        _lastRawChange = 0;
        return;
    }

    if (_lastRawChange != 0 &&
        now - _lastRawChange >= kWifiSwitchDebounceMs) {
        _stableOn      = _rawOn;
        _lastRawChange = 0;

        DBG("WIFI-SWITCH: %s", _stableOn ? "ON" : "OFF");
        if (_stableOn) {
            webStart();    // Bring SoftAP + DNS back up
        } else {
            webStop();     // Powers down the whole Wi-Fi radio
        }
    }
}
