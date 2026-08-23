// ============================================================================
// camera_manager.h — Runtime camera backend dispatcher
// ============================================================================
// Owns the shared NimBLE stack and forwards calls to the active backend
// (DJI Osmo / GoPro) selected in the Web UI.
// ============================================================================

#ifndef CAMERA_MANAGER_H
#define CAMERA_MANAGER_H

#include "camera_common.h"
#include "settings.h"

/// Initialise the BLE stack + active backend (call once from setup()).
void camInit();

/// Call from loop(). Drives scan/connect/keep-alive for the active backend.
void camUpdate();

/// Send record start/stop to the active camera.
bool camSendStartRecord();
bool camSendStopRecord();

BleConnectionState   camGetState();
const CameraTelemetry& camGetTelemetry();

/// True when the active camera can accept commands right now.
bool camIsReady();

/// Human-readable name of the active backend ("DJI Osmo" / "GoPro").
const char* camGetName();

/// Switch camera brand at runtime (disconnects current backend, restarts
/// scanning with the new one, persists the choice).
void camSetCamera(CameraType type);

/// Immediately (re)connect to the registry's active saved camera: stops any
/// scan, points the backend at that MAC and initiates the connection.
void camKick();

#endif // CAMERA_MANAGER_H
