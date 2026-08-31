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

/// Drop entries whose lastSeen is older than SCAN_RESULT_TTL_MS.  Cheap
/// O(n) sweep; called on every add.  Keeps the list fresh during a scan
/// so the UI only shows devices that are actually still nearby.
static void evictStale() {
    uint32_t now = millis();
    uint8_t w = 0;
    for (uint8_t r = 0; r < _count; r++) {
        if ((uint32_t)(now - _results[r].lastSeen) <= SCAN_RESULT_TTL_MS) {
            if (w != r) _results[w] = _results[r];
            w++;
        }
    }
    _count = w;
}

/// Find the index of the weakest-RSSI entry, or -1 if the table is empty.
static int8_t findWeakestIdx() {
    if (_count == 0) return -1;
    int8_t worst = 0;
    for (uint8_t i = 1; i < _count; i++) {
        if (_results[i].rssi < _results[worst].rssi) worst = i;
    }
    return worst;
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

void scanResultsStart() {
    // Wipe the previous table and OPEN the collection gate so the next
    // scanResultsAdd() call from the NimBLE callback is accepted.  Without
    // this, _scanning stays false and the gate `if (!_scanning && !
    // _scanComplete) return;` in scanResultsAdd() drops every advertisement.
    _count        = 0;
    _scanning     = true;
    _scanComplete = false;
    memset(_results, 0, sizeof(_results));
}

void scanResultsAdd(uint8_t type, const char *mac, const char *name, int8_t rssi) {
    if (!mac || !*mac) return;
    if (!_scanning && !_scanComplete) return; // Only collect during active scan

    evictStale();

    int8_t idx = findResultByMac(type, mac);
    uint32_t now = millis();

    if (idx >= 0) {
        // Update existing entry (always; this is a fresh sighting).
        _results[idx].rssi = rssi;
        _results[idx].lastSeen = now;
        if (name && *name) {
            strlcpy(_results[idx].name, name, sizeof(_results[idx].name));
        }
        return;
    }

    if (_count < MAX_SCAN_RESULTS) {
        // Room available — append.
        ScanResult &r = _results[_count];
        r.type = type;
        strlcpy(r.mac, mac, sizeof(r.mac));
        strlcpy(r.name, name ? name : "", sizeof(r.name));
        r.rssi = rssi;
        r.saved = false;
        r.active = false;
        r.lastSeen = now;
        _count++;
        return;
    }

    // Table is full.  Evict the weakest-RSSI entry IF the new one is
    // stronger — otherwise drop the new one.  This keeps the strongest
    // nearby devices visible without ever growing beyond MAX_SCAN_RESULTS.
    int8_t worst = findWeakestIdx();
    if (worst < 0) return;
    if (rssi <= _results[worst].rssi) {
        // New device is weaker than the weakest existing one — ignore it.
        return;
    }
    ScanResult &w = _results[worst];
    w.type = type;
    strlcpy(w.mac, mac, sizeof(w.mac));
    strlcpy(w.name, name ? name : "", sizeof(w.name));
    w.rssi = rssi;
    w.saved = false;
    w.active = false;
    w.lastSeen = now;
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

/// Copy pointers to a sorted snapshot of the scan results.  Sorted by RSSI
/// descending (strongest first), with stale entries dropped.  Stable: ties
/// preserve insertion order.  Caller-allocated buffer; bounded by `outMax`.
uint8_t scanResultsGetSortedByRssi(ScanResult const **out, uint8_t outMax) {
    if (!out || outMax == 0) return 0;

    evictStale();

    // Build a parallel index array and sort it by RSSI desc.
    uint8_t n = (_count < outMax) ? _count : outMax;
    uint8_t idx[MAX_SCAN_RESULTS];
    for (uint8_t i = 0; i < _count; i++) idx[i] = i;
    // Insertion sort — n is small (<=10) so this is fast and stack-only.
    for (uint8_t i = 1; i < _count; i++) {
        uint8_t key = idx[i];
        int8_t  krssi = _results[key].rssi;
        uint8_t j = i;
        while (j > 0 && _results[idx[j - 1]].rssi < krssi) {
            idx[j] = idx[j - 1];
            j--;
        }
        idx[j] = key;
    }
    for (uint8_t i = 0; i < n; i++) out[i] = &_results[idx[i]];
    return n;
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