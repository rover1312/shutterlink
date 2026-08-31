// ============================================================================
// gopro_camera.h — NimBLE BLE Central backend for GoPro HERO8 and newer
// ============================================================================
// Implements the public "Open GoPro" BLE API:
//   • Service 0xFEA6 (Control & Query)
//   • Encoded JSON shutter commands
//   • Registered status updates (battery %, encoding state)
//   • 3-second keep-alive
//
// First-time connections require approving the pairing prompt on the camera
// screen.  After that, the ESP32 bond is remembered and connects silently.
// ============================================================================

#ifndef GOPRO_CAMERA_H
#define GOPRO_CAMERA_H

#include "camera_common.h"

void                gpInit();
void                gpUpdate();
bool                gpSendStartRecord();
bool                gpSendStopRecord();
BleConnectionState  gpGetState();
const CameraTelemetry& gpGetTelemetry();
bool                gpIsReady();
/// Stop scanning and connect directly to this MAC (user-approved pairing).
void                gpTargetMac(const char *mac);

/// Read-only access to the last connect-attempt error (empty = no error).
/// Surfaced via /api/status and shown as a toast in the Web UI.
const char*         gpGetLastError();

#endif // GOPRO_CAMERA_H
