// ============================================================================
// scan_results.h — BLE Scan Results Collector
// ============================================================================
// Collects all devices seen during BLE scans for display in the Web UI.
// Keeps track of discovered devices with RSSI, name, type, and saved status.
// ============================================================================

#ifndef SCAN_RESULTS_H
#define SCAN_RESULTS_H

#include <Arduino.h>
#include "settings.h"

#define MAX_SCAN_RESULTS 32

struct ScanResult {
    uint8_t type;               // CameraType (CAMERA_DJI / CAMERA_GOPRO)
    char    mac[18];            // "AA:BB:CC:DD:EE:FF"
    char    name[24];           // Advertised name (may be empty)
    int8_t  rssi;               // Signal strength in dBm
    bool    saved;              // True if in saved camera registry
    bool    active;             // True if this is the active saved camera
    uint32_t lastSeen;          // millis() when last seen
};

/// Initialize the scan results collector.
void scanResultsInit();

/// Called from BLE scan callbacks to record a discovered device.
/// @param type Camera type (CAMERA_DJI / CAMERA_GOPRO)
/// @param mac MAC address string
/// @param name Device name (may be empty)
/// @param rssi Signal strength in dBm
void scanResultsAdd(uint8_t type, const char *mac, const char *name, int8_t rssi);

/// Clear all scan results (call when starting a new scan).
void scanResultsClear();

/// Get the current scan results array.
/// @param count Output parameter for number of results
/// @return Pointer to scan results array (valid until next clear)
const ScanResult* scanResultsGet(uint8_t &count);

/// Mark scan as complete (stops updating results).
void scanResultsMarkComplete();

/// Check if a scan is currently in progress.
bool scanResultsIsScanning();

/// Update saved/active status for all results based on current registry.
void scanResultsUpdateSavedStatus();

#endif // SCAN_RESULTS_H