// ============================================================================
// dji_camera.h — NimBLE BLE Central backend for DJI Osmo Action cameras
// ============================================================================
// Implements the DUML-over-BLE protocol verified by lib-osmo-ble and osmosis.
// Exposes the uniform backend interface consumed by camera_manager.
// ============================================================================

#ifndef DJI_CAMERA_H
#define DJI_CAMERA_H

#include "camera_common.h"

void                djiInit();
void                djiUpdate();
bool                djiSendStartRecord();
bool                djiSendStopRecord();
BleConnectionState  djiGetState();
const CameraTelemetry& djiGetTelemetry();
bool                djiIsReady();
/// Stop scanning and connect directly to this MAC (user-approved pairing).
void                djiTargetMac(const char *mac);

/// User-initiated one-shot discovery scan (5 s window).  Called by
/// camera_manager::camStartUserScan() from the /api/camera {scan:true}
/// endpoint — NEVER from the background reconnect loop.
void                djiStartScan();

/// Read-only access to the last connect-attempt error (empty = no error).
/// Surfaced via /api/status and shown as a toast in the Web UI.
const char*         djiGetLastError();

#endif // DJI_CAMERA_H
