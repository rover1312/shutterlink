// ============================================================================
// osd_slots.h — Betaflight Custom Message 1..4 content manager
// ============================================================================
// Formats telemetry into up to four independent strings and pushes them to
// the FC via MSP2_SET_TEXT (0x3007, types 7..10 = Custom Message 1..4).
// Position each element in the Betaflight Configurator OSD tab.
// ============================================================================

#ifndef OSD_SLOTS_H
#define OSD_SLOTS_H

#include <Arduino.h>

/// Initialise the OSD updater (call from setup()).
void osdSlotsInit();

/// Call from loop(). Pushes changed slot contents on a timer.
void osdSlotsUpdate();

/// Last formatted text for a custom-message slot (0..3), "" if blank.
const char* osdSlotText(uint8_t slot);

#endif // OSD_SLOTS_H
