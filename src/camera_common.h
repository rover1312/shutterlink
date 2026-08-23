// ============================================================================
// camera_common.h — Shared types for camera BLE backends
// ============================================================================
// Every camera backend (DJI Osmo, GoPro) implements the same function set so
// the camera manager can dispatch to the active one at runtime.
// ============================================================================

#ifndef CAMERA_COMMON_H
#define CAMERA_COMMON_H

#include <Arduino.h>
#include "config.h"

// ──────────────────────────────────────────────────────────────────────────────
// Camera State (parsed from telemetry notifications)
// ──────────────────────────────────────────────────────────────────────────────

enum CameraRecordingState : uint8_t {
    CAM_STATE_UNKNOWN   = 0,
    CAM_STATE_STANDBY   = 1,
    CAM_STATE_RECORDING = 2,
    CAM_STATE_ERROR     = 3,
};

struct CameraTelemetry {
    uint8_t              batteryPercent;    // 0–100 (255 = unknown)
    uint16_t             recTimeSeconds;    // Total recording time in seconds
    CameraRecordingState state;
    bool                 dataValid;         // True once we've received telemetry
    char                 model[24];         // Reported model name (best effort)

    CameraTelemetry()
        : batteryPercent(255), recTimeSeconds(0),
          state(CAM_STATE_UNKNOWN), dataValid(false) {
        model[0] = '\0';
    }
};

// ──────────────────────────────────────────────────────────────────────────────
// BLE Connection State
// ──────────────────────────────────────────────────────────────────────────────

enum BleConnectionState : uint8_t {
    BLE_DISCONNECTED,
    BLE_SCANNING,
    BLE_CONNECTING,
    BLE_AUTHENTICATING,   // DJI: pairing handshake / GoPro: waiting for approval
    BLE_CONNECTED,        // Ready to send commands
};

#endif // CAMERA_COMMON_H
