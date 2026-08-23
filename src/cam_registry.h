// ============================================================================
// cam_registry.h — Saved camera registry
// ============================================================================
// Cameras seen during BLE scans are remembered (brand, MAC, name) and
// persisted, so the Web UI can list them with online/offline status and let
// you pick which one to connect to.
//
// Discovery happens in the NimBLE host task; to stay lock-free the callback
// drops findings into a pending slot which is flushed into the registry from
// loop() via camRegistryProcess().
// ============================================================================

#ifndef CAM_REGISTRY_H
#define CAM_REGISTRY_H

#include <Arduino.h>
#include "settings.h"

/// Called from BLE scan callbacks (host task).  Cheap, non-blocking.
void camRegistryRemember(uint8_t type, const char *mac, const char *name);

/// Load persisted entries into the fresh settings struct (from settingsLoad).
void camRegistryLoad();

/// Call from loop(): flushes any pending discovery into the registry,
/// dedupes by MAC, persists changes, auto-activates the first camera ever
/// seen (so a fresh device just works).
void camRegistryProcess();

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
