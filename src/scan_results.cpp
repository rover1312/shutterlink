// ============================================================================
// scan_results.cpp — BLE Scan Results Collector Implementation
// ============================================================================

#include "scan_results.h"
#include "cam_registry.h"
#include "settings.h"
#include "camera_common.h"

// ──────────────────────────────────────────────────────────────────────────────
// Internal state
// ──────────────────────────────────────────────────────────────────────────────

static ScanResult _results[MAX_SCAN_RESULTS];
static uint8_t _count = 0;
static bool _scanning = false;
static bool _scanComplete = false;

static bool macEquals(const char *a, const char *b) {
    return a && b && strcasecmp(a, b) == 0;
}

static int8_t findResultByMac(uint8_t type, const char *mac) {
    for (uint8_t i = 0; i < _count; i++) {
        if (_results[i].type == type && macEquals(_results[i].mac, mac)) {
            return i;
        }
    }
    return -1;
}

// ──────────────────────────────────────────────────────────────────────────────
// Public API
// ──────────────────────────────────────────────────────────────────────────────

void scanResultsInit() {
    _count = 0;
    _scanning = false;
    _scanComplete = false;
    memset(_results, 0, sizeof(_results));
}

void scanResultsAdd(uint8_t type, const char *mac, const char *name, int8_t rssi) {
    if (!mac || !*mac) return;
    if (!_scanning && !_scanComplete) return; // Only collect during active scan

    int8_t idx = findResultByMac(type, mac);
    uint32_t now = millis();

    if (idx >= 0) {
        // Update existing entry
        _results[idx].rssi = rssi;
        _results[idx].lastSeen = now;
        if (name && *name) {
            strlcpy(_results[idx].name, name, sizeof(_results[idx].name));
        }
    } else if (_count < MAX_SCAN_RESULTS) {
        // Add new entry
        ScanResult &r = _results[_count];
        r.type = type;
        strlcpy(r.mac, mac, sizeof(r.mac));
        strlcpy(r.name, name ? name : "", sizeof(r.name));
        r.rssi = rssi;
        r.saved = false;
        r.active = false;
        r.lastSeen = now;
        _count++;
    }
}

void scanResultsClear() {
    _count = 0;
    _scanning = true;
    _scanComplete = false;
    memset(_results, 0, sizeof(_results));
}

const ScanResult* scanResultsGet(uint8_t &count) {
    count = _count;
    return _results;
}

void scanResultsMarkComplete() {
    _scanning = false;
    _scanComplete = true;
}

bool scanResultsIsScanning() {
    return _scanning;
}

void scanResultsUpdateSavedStatus() {
    ShutterSettings &s = settingsGet();

    // Find active camera index
    int8_t activeIdx = -1;
    for (uint8_t i = 0; i < s.camCount; i++) {
        if (s.cams[i].active) {
            activeIdx = i;
            break;
        }
    }

    for (uint8_t i = 0; i < _count; i++) {
        _results[i].saved = false;
        _results[i].active = false;

        for (uint8_t j = 0; j < s.camCount; j++) {
            if (s.cams[j].type == _results[i].type &&
                macEquals(s.cams[j].mac, _results[i].mac)) {
                _results[i].saved = true;
                if ((int8_t)j == activeIdx) {
                    _results[i].active = true;
                }
                break;
            }
        }
    }
}