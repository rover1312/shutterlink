// ============================================================================
// wifiswitch.h — Toggle the Web-UI Wi-Fi AP from a spare radio switch
// ============================================================================
// Watches a configurable AUX channel (same threshold as the record switch)
// and powers the SoftAP (and the Wi-Fi radio entirely) up/down.
//
//   • OFF saves ~60–100 mA — meaningful in flight, harmless on the bench.
//   • The AP always comes up at boot so you can never lock yourself out;
//     if the switch is configured and low, it turns off as soon as RC data
//     arrives (~200 ms after boot).
//
// NOTE: BLE keeps running independently — camera control is unaffected.
// ============================================================================

#ifndef WIFI_SWITCH_H
#define WIFI_SWITCH_H

#include <Arduino.h>

/// Call from setup().
void wifiSwitchInit();

/// Feed the latest raw value of the configured channel in µs (from MSP_RC).
void wifiSwitchFeedRc(uint16_t rcValueUsec);

/// Call from loop(). Applies debounced switch transitions to the radio.
void wifiSwitchUpdate();

#endif // WIFI_SWITCH_H
