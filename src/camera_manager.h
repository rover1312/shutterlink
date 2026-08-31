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

/// Switch camera brand at runtime (disconnects current backend, persists
/// the choice).  Does NOT start a discovery scan — only changes which
/// backend is initialised.
void camSetCamera(CameraType type);

/// Immediately (re)connect to the registry's active saved camera: stops any
/// scan, points the backend at that MAC and initiates the connection.
void camKick();

/// User-initiated discovery scan.  Switches the backend to the brand pill
/// the user picked and calls that backend's startScan() once.  This is the
/// ONLY entry point for a discovery scan — djiUpdate()/gpUpdate() will
/// NOT auto-restart the scan after the 5 s window closes.
void camStartUserScan();

#endif // CAMERA_MANAGER_H
