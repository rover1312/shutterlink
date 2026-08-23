// ============================================================================
// settings.cpp — NVS-backed persistent configuration
// ============================================================================

#include "settings.h"
#include "cam_registry.h"
#include <Preferences.h>

static ShutterSettings _s;
static Preferences _prefs;

static void applyDefaults() {
    _s.camera          = (CameraType)DEFAULT_CAMERA_TYPE;
    _s.auxChannelIndex = DEFAULT_AUX_CHANNEL_INDEX;
    _s.rcThresholdUs   = DEFAULT_RC_THRESHOLD_US;
    _s.debounceMs      = DEFAULT_RC_DEBOUNCE_MS;
    _s.recordOnArm     = DEFAULT_RECORD_ON_ARM;
    _s.stopOnDisarm    = DEFAULT_STOP_ON_DISARM;
    _s.wifiSwitchCh    = DEFAULT_WIFI_SWITCH_CH;

    strlcpy(_s.apSsid, WIFI_AP_DEFAULT_SSID, sizeof(_s.apSsid));
    strlcpy(_s.apPass, WIFI_AP_DEFAULT_PASS, sizeof(_s.apPass));

    for (int i = 0; i < 4; i++) {
        _s.osdSlot[i] = DEFAULT_OSD_SLOT_1 + i;
    }
    _s.camCount = 0;
    memset(_s.cams, 0, sizeof(_s.cams));
}

void settingsLoad() {
    applyDefaults();

    if (!_prefs.begin("shutterlink", false)) {
        DBG("SETTINGS: NVS open failed — using defaults");
        return;
    }

    _s.camera          = (CameraType)_prefs.getUChar("camera", _s.camera);
    _s.auxChannelIndex = _prefs.getUChar("auxCh", _s.auxChannelIndex);
    _s.rcThresholdUs   = _prefs.getUShort("thr", _s.rcThresholdUs);
    _s.debounceMs      = _prefs.getUShort("deb", _s.debounceMs);
    _s.recordOnArm     = _prefs.getBool("roa", _s.recordOnArm);
    _s.stopOnDisarm    = _prefs.getBool("sod", _s.stopOnDisarm);
    _s.wifiSwitchCh    = _prefs.getUChar("wifiCh", _s.wifiSwitchCh);

    char buf[65] = {0};
    if (_prefs.getString("ssid", buf, sizeof(buf)) > 0) strlcpy(_s.apSsid, buf, sizeof(_s.apSsid));
    buf[0] = 0;
    if (_prefs.getString("apkey", buf, sizeof(buf)) > 0) strlcpy(_s.apPass, buf, sizeof(_s.apPass));

    for (int i = 0; i < 4; i++) {
        char key[8];
        snprintf(key, sizeof(key), "slot%d", i);
        uint8_t v = _prefs.getUChar(key, _s.osdSlot[i]);
        if (v < OSD_SLOT_COUNT) _s.osdSlot[i] = v;
    }

    // Sanity clamps
    if (_s.auxChannelIndex > 15)            _s.auxChannelIndex = 15;
    if (_s.rcThresholdUs < 1200)            _s.rcThresholdUs = 1200;
    if (_s.rcThresholdUs > 1800)            _s.rcThresholdUs = 1800;
    if (_s.debounceMs < 50)                 _s.debounceMs = 50;
    if (_s.debounceMs > 1000)               _s.debounceMs = 1000;
    if (_s.wifiSwitchCh > 15 && _s.wifiSwitchCh != 255)
                                            _s.wifiSwitchCh = 255;

    _prefs.end();

    // Saved camera registry lives in the same NVS namespace.
    camRegistryLoad();

    DBG("SETTINGS: Loaded (cam=%u aux=%u thr=%u deb=%u roa=%d)",
        _s.camera, _s.auxChannelIndex, _s.rcThresholdUs, _s.debounceMs, _s.recordOnArm);
}

void settingsSave() {
    if (!_prefs.begin("shutterlink", false)) {
        DBG("SETTINGS: NVS open failed — not saved");
        return;
    }
    _prefs.putUChar("camera", _s.camera);
    _prefs.putUChar("auxCh", _s.auxChannelIndex);
    _prefs.putUShort("thr", _s.rcThresholdUs);
    _prefs.putUShort("deb", _s.debounceMs);
    _prefs.putBool("roa", _s.recordOnArm);
    _prefs.putBool("sod", _s.stopOnDisarm);
    _prefs.putUChar("wifiCh", _s.wifiSwitchCh);
    _prefs.putString("ssid", _s.apSsid);
    _prefs.putString("apkey", _s.apPass);

    for (int i = 0; i < 4; i++) {
        char key[8];
        snprintf(key, sizeof(key), "slot%d", i);
        _prefs.putUChar(key, _s.osdSlot[i]);
    }

    _prefs.end();
    DBG("SETTINGS: Saved");
}

void settingsReset() {
    applyDefaults();
}

ShutterSettings& settingsGet() {
    return _s;
}

const char* cameraTypeName(CameraType type) {
    return (type == CAMERA_GOPRO) ? "GoPro" : "DJI Osmo";
}
