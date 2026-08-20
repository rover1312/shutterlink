// ============================================================================
// dji_ble_client.h — NimBLE BLE Central for DJI Action 2 camera
// ============================================================================
// Handles:
//   • BLE scanning for DJI cameras
//   • Connection management with auto-reconnect
//   • GATT service/characteristic discovery
//   • DJI authentication handshake
//   • Start/Stop record commands
//   • Telemetry notification parsing (battery, rec time, state)
//   • Keep-alive pings
// ============================================================================

#ifndef DJI_BLE_CLIENT_H
#define DJI_BLE_CLIENT_H

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
    uint8_t              batteryPercent;    // 0–100
    uint16_t             recTimeSeconds;    // Total recording time in seconds
    CameraRecordingState state;
    bool                 dataValid;         // True once we've received at least one notification
};

// ──────────────────────────────────────────────────────────────────────────────
// BLE Connection State
// ──────────────────────────────────────────────────────────────────────────────

enum BleConnectionState : uint8_t {
    BLE_DISCONNECTED,
    BLE_SCANNING,
    BLE_CONNECTING,
    BLE_AUTHENTICATING,
    BLE_CONNECTED,      // Authenticated and ready to send commands
};

// ──────────────────────────────────────────────────────────────────────────────
// Public API
// ──────────────────────────────────────────────────────────────────────────────

/// Initialise NimBLE stack and set up callbacks.
void bleInit();

/// Call from loop(). Manages scanning, connection, auth, and keep-alive.
/// Non-blocking — uses millis()-based timers internally.
void bleUpdate();

/// Send "Start Recording" command to the camera.
/// Returns true if the write was dispatched successfully.
bool bleSendStartRecord();

/// Send "Stop Recording" command to the camera.
/// Returns true if the write was dispatched successfully.
bool bleSendStopRecord();

/// Get the current BLE connection state.
BleConnectionState bleGetState();

/// Get the latest camera telemetry snapshot.
/// Returns a const reference to an internal struct updated by BLE notifications.
const CameraTelemetry& bleGetTelemetry();

/// Returns true if the camera is connected AND authenticated (ready for commands).
bool bleIsReady();

/// Format a human-readable OSD string from telemetry.
///   outBuf     : destination buffer (must be at least `bufLen` bytes).
///   bufLen     : size of outBuf.
/// Example output: "REC 85% 12:34" or "STBY 50% 05:12" or "CAM: OFF".
void bleFormatOSDString(char *outBuf, size_t bufLen);

#endif // DJI_BLE_CLIENT_H
