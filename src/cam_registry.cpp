// ============================================================================
// cam_registry.cpp — Saved camera registry
// ============================================================================
//
// Two-tier design:
//   • _discovered[]  — in-RAM only; populated by camRegistryRemember() during
//     BLE scans. Used by the Web UI to show "Discovered cameras" (Card 3).
//     NEVER auto-persisted to NVS. Cleared on reboot (intentional).
//   • s.cams[]       — persisted to NVS. Only written when the user clicks
//     "Pair & Save" on a discovered camera, which calls camRegistrySave().
//
// This fixes the "ghost device" bug: neighbour's cameras that the radio
// briefly sees will appear in Card 3 as transient discovered devices, but
// will NOT be silently written to flash.
// ============================================================================

#include "cam_registry.h"
#include "camera_manager.h"
#include <Preferences.h>

// ──────────────────────────────────────────────────────────────────────────────
// Pending discovery slot (written from BLE host task, drained from loop)
// Lock-free: only writes to a static struct, no NVS, no allocation.
// ──────────────────────────────────────────────────────────────────────────────

static struct {
    bool    flag;
    uint8_t type;
    char    mac[18];
    char    name[24];
} _pending;

static ScanResult _discovered[MAX_SCAN_RESULTS];
static uint8_t    _discoveredCount = 0;

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

/// Drain the BLE host task's pending slot into the in-RAM _discovered[] array.
/// Does NOT touch NVS. Does NOT auto-activate anything. Safe to call from
/// loop() (not from the NimBLE host task).
void camRegistryProcess() {
    if (!_pending.flag) return;
    _pending.flag = false;

    // Add/update entry in the in-RAM discovered list (for the Web UI).
    int8_t found = -1;
    for (uint8_t i = 0; i < _discoveredCount; i++) {
        if (_discovered[i].type == _pending.type &&
            macEquals(_discovered[i].mac, _pending.mac)) { found = i; break; }
    }
    if (found >= 0) {
        if (_discovered[found].name[0] == 0 && _pending.name[0]) {
            strlcpy(_discovered[found].name, _pending.name, sizeof(_discovered[found].name));
        }
    } else if (_discoveredCount < MAX_SCAN_RESULTS) {
        ScanResult &d = _discovered[_discoveredCount++];
        d.type = _pending.type;
        d.rssi = 0;
        strlcpy(d.mac, _pending.mac, sizeof(d.mac));
        strlcpy(d.name, _pending.name, sizeof(d.name));
    }
}

/// Read-only access to the in-RAM discovered list. Used by web_server.cpp
/// to send the pending_cams/discovered array to the Web UI.
const ScanResult* camRegistryDiscovered(uint8_t &count) {
    count = _discoveredCount;
    return _discovered;
}

/// Clear the in-RAM discovered list (called when the user starts a new scan).
void camRegistryClearDiscovered() {
    _discoveredCount = 0;
    memset(_discovered, 0, sizeof(_discovered));
}

/// Explicitly save a discovered camera to NVS. Called when the user clicks
/// "Pair & Save" in the Web UI. Returns false on full registry or invalid MAC.
bool camRegistrySave(uint8_t type, const char *mac, const char *name) {
    if (!mac || !*mac) return false;
    if (type != CAMERA_DJI && type != CAMERA_GOPRO) return false;

    ShutterSettings &s = settingsGet();

    // Dedup: if this MAC is already saved, just refresh name and return ok.
    for (uint8_t i = 0; i < s.camCount; i++) {
        if (macEquals(s.cams[i].mac, mac) && s.cams[i].type == type) {
            if (name && *name) {
                strlcpy(s.cams[i].name, name, sizeof(s.cams[i].name));
                persist();
            }
            return true;
        }
    }
    if (s.camCount >= MAX_SAVED_CAMERAS) return false;

    SavedCamera &c = s.cams[s.camCount++];
    c.type   = type;
    c.active = false;            // Don't auto-activate; user picks later
    strlcpy(c.mac,  mac,  sizeof(c.mac));
    strlcpy(c.name, name && *name ? name : mac, sizeof(c.name));
    persist();
    DBG("REGISTRY: saved %s (%s) — user-approved",
        c.name, c.mac);
    return true;
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

bool camRegistryActiveMac(uint8_t type, char *out, size_t outLen) {
    if (!out || outLen == 0) return false;
    out[0] = '\0';
    ShutterSettings &s = settingsGet();
    for (uint8_t i = 0; i < s.camCount; i++) {
        if (s.cams[i].active && s.cams[i].type == type) {
            strlcpy(out, s.cams[i].mac, outLen);
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
