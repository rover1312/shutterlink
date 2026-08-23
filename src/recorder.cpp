// ============================================================================
// recorder.cpp — Recording decision engine
// ============================================================================

#include "recorder.h"
#include "config.h"
#include "settings.h"
#include "fc_status.h"
#include "camera_manager.h"

// ──────────────────────────────────────────────────────────────────────────────
// Internal state
// ──────────────────────────────────────────────────────────────────────────────

// RC switch tracking
static bool     _switchIsOn       = false;   // Current debounced state
static bool     _switchRawIsOn    = false;   // Raw (un-debounced) state
static uint32_t _switchLastChange = 0;       // millis() of last raw change
static uint16_t _lastRcValue      = 0;       // Last read RC channel value (µs)
static bool     _rcDataValid      = false;   // True after first valid MSP_RC

// Record-on-arm latch + suppression after manual stop
static bool     _roaLatched       = false;   // Latched by arming
static bool     _roaSuppress      = false;   // Manual stop while armed

// Desired state / last sent command
static bool     _desired          = false;
static int8_t   _pendingCmd       = 0;    // 1 = start pending retry, -1 = stop

static bool     _lastArmed        = false;

// ──────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ──────────────────────────────────────────────────────────────────────────────

static void sendStart() {
    if (camSendStartRecord()) {
        DBG("REC: start sent");
        _pendingCmd = 0;
    } else {
        DBG("REC: start deferred — camera not ready");
        _pendingCmd = 1;
    }
}

static void sendStop() {
    if (camSendStopRecord()) {
        DBG("REC: stop sent");
        _pendingCmd = 0;
    } else {
        DBG("REC: stop deferred — camera not ready");
        _pendingCmd = -1;
    }
}

/// Clear the record-on-arm latch when the pilot signals "stop" intent.
static void clearRoaIntent() {
    if (_roaLatched) DBG("REC: record-on-arm latch cleared");
    _roaLatched  = false;
    _roaSuppress = false;
}

// ──────────────────────────────────────────────────────────────────────────────
// Public API
// ──────────────────────────────────────────────────────────────────────────────

void recorderInit() {
    _switchIsOn = false;
    _rcDataValid = false;
}

void recorderFeedRcValue(uint16_t rcValueUsec) {
    _lastRcValue = rcValueUsec;
    _rcDataValid = true;

    bool rawOn = (rcValueUsec > settingsGet().rcThresholdUs);
    if (rawOn != _switchRawIsOn) {
        _switchRawIsOn    = rawOn;
        _switchLastChange = millis();
    }
}

void recorderUpdate() {
    const ShutterSettings &cfg = settingsGet();
    uint32_t now = millis();

    // ── Debounce the RC switch ──────────────────────────────────────────
    if (_rcDataValid &&
        _switchRawIsOn != _switchIsOn &&
        (now - _switchLastChange >= cfg.debounceMs)) {
        _switchIsOn = _switchRawIsOn;

        DBG("SWITCH: ch%d %s (%u µs)", cfg.auxChannelIndex,
            _switchIsOn ? "ON" : "OFF", _lastRcValue);

        // Explicit user intent via the switch always clears the RoA latch.
        clearRoaIntent();
    }

    // ── Arm-state edges ─────────────────────────────────────────────────
    const FcTelemetry &fc = fcGetTelemetry();
    static bool armValid = false;

    if (fc.fcAlive && (!armValid || fc.armed != _lastArmed)) {
        bool rising = fc.armed && !_lastArmed;
        _lastArmed = fc.armed;
        armValid   = true;

        DBG("FC: %s", fc.armed ? "ARMED" : "DISARMED");

        if (rising && cfg.recordOnArm && !_switchIsOn && !_roaSuppress) {
            DBG("REC: record-on-arm engaged");
            _roaLatched = true;
        }
        if (!rising && cfg.stopOnDisarm) {
            clearRoaIntent();   // Disarm stops recording (default behaviour)
        }
        if (!fc.armed) {
            _roaSuppress = false;  // Fresh arming may latch again
        }
    }

    // ── Compute desired state ───────────────────────────────────────────
    bool newDesired = _switchIsOn || _roaLatched;

    if (newDesired != _desired) {
        _desired = newDesired;
        if (_desired) sendStart(); else sendStop();
    }

    // ── Retry a deferred command once the camera becomes ready ─────────
    // Camera commands are absolute (start/stop), never toggles, so a retry
    // after reconnect is always safe.  Backed off to at most one retry per
    // second.
    static uint32_t lastRetry = 0;
    if (_pendingCmd != 0 && camIsReady() && now - lastRetry >= 1000) {
        lastRetry = now;
        if (_pendingCmd > 0) {
            sendStart();
        } else {
            sendStop();
        }
    }
}

void recorderManualStart() {
    // Manual start: behave like a fresh desired=true edge.
    clearRoaIntent();
    if (settingsGet().recordOnArm) {
        _roaLatched = true;   // Keep it running until switch-off/manual stop
    }
    if (!_desired) _desired = true;
    sendStart();
}

void recorderManualStop() {
    clearRoaIntent();
    _desired = false;
    // If armed with record-on-arm enabled and no stop-on-disarm, prevent an
    // immediate re-latch until the FC disarms or the switch is used.
    if (settingsGet().recordOnArm && fcGetTelemetry().armed &&
        !settingsGet().stopOnDisarm) {
        _roaSuppress = true;
    }
    sendStop();
}

bool recorderSwitchOn()         { return _switchIsOn; }
bool recorderDesiredRecording() { return _desired; }
uint16_t recorderLastRcValue()  { return _lastRcValue; }
