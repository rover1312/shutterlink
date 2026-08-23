// ============================================================================
// cam_registry.cpp — Saved camera registry
// ============================================================================

#include "cam_registry.h"
#include "camera_manager.h"
#include <Preferences.h>

// ──────────────────────────────────────────────────────────────────────────────
// Pending discovery slot (written from BLE host task, drained from loop)
// ──────────────────────────────────────────────────────────────────────────────

static struct {
    bool    flag;
    uint8_t type;
    char    mac[18];
    char    name[24];
} _pending;

void camRegistryRemember(uint8_t type, const char *mac, const char *name) {
    if (!mac || !*mac) return;
    _pending.type = type;
    strlcpy(_pending.mac, mac, sizeof(_pending.mac));
    strlcpy(_pending.name, name ? name : "", sizeof(_pending.name));
    _pending.flag = true;   // Overwrite stale pending entry — newest wins
}

// ──────────────────────────────────────────────────────────────────────────────
// Registry maintenance
// ──────────────────────────────────────────────────────────────────────────────

static bool macEquals(const char *a, const char *b) {
    return a && b && strcasecmp(a, b) == 0;
}

static void persist() {
    Preferences prefs;
    if (!prefs.begin("shutterlink", false)) return;

    ShutterSettings &s = settingsGet();
    prefs.putUChar("camCnt", s.camCount);
    for (uint8_t i = 0; i < MAX_SAVED_CAMERAS; i++) {
        char key[10];
        snprintf(key, sizeof(key), "cT%d", i);
        prefs.putUChar(key, s.cams[i].type);
        snprintf(key, sizeof(key), "cM%d", i);
        prefs.putString(key, s.cams[i].mac);
        snprintf(key, sizeof(key), "cN%d", i);
        prefs.putString(key, s.cams[i].name);
        snprintf(key, sizeof(key), "cA%d", i);
        prefs.putBool(key, s.cams[i].active);
    }
    prefs.end();
}

/// Public loader — called from settingsLoad().
void camRegistryLoad() {
    Preferences prefs;
    ShutterSettings &s = settingsGet();
    memset(s.cams, 0, sizeof(s.cams));
    s.camCount = 0;
    if (!prefs.begin("shutterlink", true)) return;

    uint8_t cnt = prefs.getUChar("camCnt", 0);
    if (cnt > MAX_SAVED_CAMERAS) cnt = MAX_SAVED_CAMERAS;
    for (uint8_t i = 0; i < cnt; i++) {
        char key[10];
        snprintf(key, sizeof(key), "cT%d", i);
        uint8_t t = prefs.getUChar(key, 255);
        if (t != CAMERA_DJI && t != CAMERA_GOPRO) continue;
        snprintf(key, sizeof(key), "cM%d", i);
        String m = prefs.getString(key, "");
        if (!m.length()) continue;
        s.cams[s.camCount].type = t;
        s.cams[s.camCount].active = false;
        strlcpy(s.cams[s.camCount].mac, m.c_str(), sizeof(s.cams[i].mac));
        snprintf(key, sizeof(key), "cN%d", i);
        String n = prefs.getString(key, "");
        strlcpy(s.cams[s.camCount].name, n.c_str(), sizeof(s.cams[i].name));
        snprintf(key, sizeof(key), "cA%d", i);
        s.cams[s.camCount].active = prefs.getBool(key, false);
        s.camCount++;
    }
    // Guarantee exactly one active (or none if list empty).
    bool anyActive = false;
    for (uint8_t i = 0; i < s.camCount; i++) anyActive |= s.cams[i].active;
    if (!anyActive && s.camCount) s.cams[0].active = true;
    prefs.end();
}

void camRegistryProcess() {
    if (!_pending.flag) return;
    _pending.flag = false;

    ShutterSettings &s = settingsGet();

    // Dedupe by MAC: refresh name/brand of an existing entry.
    int8_t found = -1;
    for (uint8_t i = 0; i < s.camCount; i++) {
        if (macEquals(s.cams[i].mac, _pending.mac)) { found = i; break; }
    }

    bool changed = false;
    if (found >= 0) {
        if (strncmp(s.cams[found].name, _pending.name, sizeof(_pending.name)) != 0 ||
            s.cams[found].type != _pending.type) {
            s.cams[found].type = _pending.type;
            strlcpy(s.cams[found].name, _pending.name, sizeof(s.cams[found].name));
            changed = true;
        }
    } else if (s.camCount < MAX_SAVED_CAMERAS) {
        SavedCamera &c = s.cams[s.camCount];
        c.type = _pending.type;
        c.active = false;   // NEVER auto-activate — pairing is user-approved
        strlcpy(c.mac, _pending.mac, sizeof(c.mac));
        strlcpy(c.name, _pending.name, sizeof(c.name));
        s.camCount++;
        changed = true;
        DBG("REGISTRY: discovered %s (%s) — tap Use in the UI to pair",
            _pending.name[0] ? _pending.name : _pending.mac,
            cameraTypeName((CameraType)_pending.type));
    }

    // NOTE: deliberately no auto-activation here. New cameras are only ever
    // paired after the user selects them ("Use") from the Web UI.
    if (changed) persist();
}

bool camRegistryMayAutoConnect(uint8_t type, const char *mac) {
    if (!mac || !*mac) return false;
    ShutterSettings &s = settingsGet();
    for (uint8_t i = 0; i < s.camCount; i++) {
        if (s.cams[i].type == type && s.cams[i].active &&
            macEquals(s.cams[i].mac, mac)) {
            return true;
        }
    }
    return false;
}

bool camRegistrySelect(uint8_t index) {
    ShutterSettings &s = settingsGet();
    if (index >= s.camCount) return false;

    if (s.cams[index].active) return true;   // Nothing to do

    for (uint8_t i = 0; i < s.camCount; i++) s.cams[i].active = (i == index);
    camSetCamera((CameraType)s.cams[index].type);   // Also persists settings
    persist();
    DBG("REGISTRY: selected %s", s.cams[index].mac);
    return true;
}

bool camRegistryRemove(uint8_t index) {
    ShutterSettings &s = settingsGet();
    if (index >= s.camCount) return false;

    bool wasActive = s.cams[index].active;
    for (uint8_t i = index; i < s.camCount - 1; i++) {
        s.cams[i] = s.cams[i + 1];
    }
    s.camCount--;
    memset(&s.cams[s.camCount], 0, sizeof(SavedCamera));

    if (wasActive) {
        // Fall back to another saved camera of any brand, if present.
        for (uint8_t i = 0; i < s.camCount; i++) s.cams[i].active = false;
        if (s.camCount) {
            s.cams[0].active = true;
            if (settingsGet().camera != s.cams[0].type) {
                camSetCamera((CameraType)s.cams[0].type);
            }
        }
    }
    persist();
    return true;
}
