// ============================================================================
// settings.h — Persistent runtime configuration (NVS via Preferences)
// ============================================================================
// Everything the user can change from the Web UI lives here and survives
// reboots.  Defaults come from config.h.
//
//   • Camera brand selection (DJI Osmo / GoPro HERO8+)
//   • Record switch: RC channel index, threshold, debounce
//   • Record-on-arm (+ optional stop on disarm)
//   • Wi-Fi AP credentials for the Web UI
//   • OSD content assignment for Custom Message slots 1..4
// ============================================================================

#ifndef SETTINGS_H
#define SETTINGS_H

#include <Arduino.h>
#include "config.h"

// ──────────────────────────────────────────────────────────────────────────────
// Camera brands
// ──────────────────────────────────────────────────────────────────────────────

enum CameraType : uint8_t {
    CAMERA_DJI   = 0,   // DJI Osmo Action family (DUML over BLE)
    CAMERA_GOPRO = 1,   // GoPro HERO8+ (Open GoPro BLE API)
};

// ──────────────────────────────────────────────────────────────────────────────
// OSD slot contents (which info goes into Betaflight Custom Message 1..4)
// ──────────────────────────────────────────────────────────────────────────────

enum OsdSlotContent : uint8_t {
    OSD_SLOT_OFF        = 0,   // Leave this custom message untouched/empty
    OSD_SLOT_CAM_STATUS = 1,   // "REC 85% 12:34" / "STBY ..." (combined)
    OSD_SLOT_REC_TIME   = 2,   // "REC 12:34"
    OSD_SLOT_BATTERY    = 3,   // "BAT 85%"
    OSD_SLOT_LINK       = 4,   // "LINK READY" / "SCAN" / "OFF"
    OSD_SLOT_FC_BATT    = 5,   // "FC 15.8V"
    OSD_SLOT_ARM_STATE  = 6,   // "ARMED" / "DISARMED"

    OSD_SLOT_COUNT      = 7,
};

// ──────────────────────────────────────────────────────────────────────────────
// Saved camera registry (auto-learned during scans, persisted)
// ──────────────────────────────────────────────────────────────────────────────

#define MAX_SAVED_CAMERAS 4

struct SavedCamera {
    uint8_t type;              // CameraType
    bool    active;            // The one the ESP32 should connect to
    char    mac[18];           // "AA:BB:CC:DD:EE:FF"
    char    name[24];          // Advertised name (may be empty)
};

// ──────────────────────────────────────────────────────────────────────────────
// Settings container
// ──────────────────────────────────────────────────────────────────────────────

struct ShutterSettings {
    CameraType camera;              // Active camera backend
    uint8_t    auxChannelIndex;     // RC channel used as record switch (0-based)
    uint16_t   rcThresholdUs;       // µs above which the switch is ON
    uint16_t   debounceMs;          // Switch debounce time
    bool       recordOnArm;         // Start recording when FC arms
    bool       stopOnDisarm;        // Stop recording when FC disarms (needs recordOnArm)
    char       apSsid[33];          // SoftAP SSID for the Web UI
    char       apPass[65];          // SoftAP password (min 8 chars, or empty = open)
    uint8_t    osdSlot[4];          // OsdSlotContent for Custom Message 1..4
    uint8_t    wifiSwitchCh;        // RC channel toggling Wi-Fi (255 = disabled)

    SavedCamera cams[MAX_SAVED_CAMERAS];
    uint8_t     camCount;
};

/// Load settings from NVS (or defaults on first boot).
void settingsLoad();

/// Persist current settings to NVS.
void settingsSave();

/// Reset to compile-time defaults (does not save automatically).
void settingsReset();

/// Access the live settings struct.
ShutterSettings& settingsGet();

/// Human-readable name of a camera type ("DJI Osmo" / "GoPro").
const char* cameraTypeName(CameraType type);

#endif // SETTINGS_H
