// ============================================================================
// cam_registry.h — Saved camera registry
// ============================================================================
// Two-tier design (see cam_registry.cpp for the rationale):
//   • In-RAM "discovered" list: populated by the BLE scan callback.  Used
//     only by the Web UI to show "Discovered cameras".  NEVER auto-persisted.
//   • NVS "saved" list: written only when the user clicks "Pair & Save" on a
//     discovered camera (or via the "Connect" button in the Web UI).
//
// This eliminates the "ghost device" bug: any camera the radio briefly sees
// appears as a discovered device but is NOT written to flash without the
// user's explicit consent.
// ============================================================================

#ifndef CAM_REGISTRY_H
#define CAM_REGISTRY_H

#include <Arduino.h>
#include "settings.h"
#include "scan_results.h"   // for ScanResult

/// Called from BLE scan callbacks (host task).  Cheap, non-blocking.
/// Pushes the MAC into a static pending slot + into the in-RAM discovered list.
void camRegistryRemember(uint8_t type, const char *mac, const char *name);

/// Load persisted entries into the fresh settings struct (from settingsLoad).
void camRegistryLoad();

/// Call from loop(): flushes any pending discovery into the in-RAM
/// discovered list.  Does NOT touch NVS.
void camRegistryProcess();

/// Read-only access to the in-RAM discovered list (Card 3 of the Web UI).
const ScanResult* camRegistryDiscovered(uint8_t &count);

/// Clear the in-RAM discovered list (called when a new scan starts).
void camRegistryClearDiscovered();

/// Explicitly save a discovered camera to NVS.  Called by the Web UI's
/// "Pair & Save" button.  Returns false on full registry or invalid args.
bool camRegistrySave(uint8_t type, const char *mac, const char *name);

/// Select a saved camera as active (switches backend brand if needed).
/// Returns false if index is invalid.
bool camRegistrySelect(uint8_t index);

/// Remove a saved camera.  If it was active, the first remaining entry
/// becomes active (and its brand becomes the scan target).
bool camRegistryRemove(uint8_t index);

/// True when `mac` belongs to a SAVED entry of exactly this type AND that
/// entry is the active one — i.e. auto-connecting to it is allowed.
bool camRegistryMayAutoConnect(uint8_t type, const char *mac);

#endif // CAM_REGISTRY_H
