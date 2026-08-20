// ============================================================================
// config.h — Project-wide configuration, pin definitions, and tunables
// ============================================================================
// ESP32-C3 ShutterLink: Betaflight ↔ DJI Action 2 bridge.
// ============================================================================

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ──────────────────────────────────────────────────────────────────────────────
// UART / MSP Configuration
// ──────────────────────────────────────────────────────────────────────────────

// Hardware UART pins for Serial1 (connected to Betaflight FC).
// On the ESP32-C3 DevKitM-1, any GPIO can be mapped to UART1.
// Connect FC TX → ESP32 RX_PIN, FC RX → ESP32 TX_PIN.
#define FC_UART_RX_PIN        20   // GPIO20 — receives data FROM the FC
#define FC_UART_TX_PIN        21   // GPIO21 — sends data TO the FC

// UART baud rate — must match Betaflight serial port config.
// Betaflight default MSP baud is 115200.
#define FC_UART_BAUD          115200

// ──────────────────────────────────────────────────────────────────────────────
// RC Channel / Switch Configuration
// ──────────────────────────────────────────────────────────────────────────────

// Zero-based index of the RC channel used as the "Record" switch.
// In Betaflight, AUX1 = channel 5 (index 4), AUX4 = channel 8 (index 7), etc.
// Index 12 = AUX 9 (channel 13). Adjust to match your Betaflight Modes tab.
#define AUX_CHANNEL_INDEX     8    // AUX4 — change to match your setup

// Threshold (µs) above which the switch is considered ON (record).
// Standard RC range is 1000-2000 µs. Mid-point is 1500.
#define RC_SWITCH_THRESHOLD   1500

// Debounce duration in milliseconds for the RC switch.
// Prevents spurious toggles from noisy RC signals.
#define RC_DEBOUNCE_MS        300

// ──────────────────────────────────────────────────────────────────────────────
// MSP Timing
// ──────────────────────────────────────────────────────────────────────────────

// How often to poll the FC for RC channel data (milliseconds).
#define MSP_RC_POLL_INTERVAL_MS     200

// Timeout waiting for an MSP response (milliseconds).
#define MSP_RESPONSE_TIMEOUT_MS     500

// ──────────────────────────────────────────────────────────────────────────────
// OSD Configuration
// ──────────────────────────────────────────────────────────────────────────────

// How often to push OSD text to the FC (milliseconds).
#define OSD_UPDATE_INTERVAL_MS      1000

// OSD text position on the HD OSD grid.
// DJI HD OSD grid is typically 50 columns × 18 rows (0-indexed).
#define OSD_TEXT_ROW          14   // Near the bottom
#define OSD_TEXT_COL          1    // Left-aligned

// Text type for MSP2_COMMON_SET_TEXT.
// 0 = PILOT_NAME, 1 = CRAFT_NAME, 2 = PID_PROFILE_NAME, 3 = RATE_PROFILE,
// 4 = BUILDKEY, 5 = BOARD_NAME.  We use PILOT_NAME for custom OSD text.
#define OSD_TEXT_TYPE         4

// Maximum OSD text length (Betaflight caps pilot name at 16 chars typically).
#define OSD_MAX_TEXT_LEN      16

// ──────────────────────────────────────────────────────────────────────────────
// BLE Configuration
// ──────────────────────────────────────────────────────────────────────────────

// BLE scan duration in seconds (0 = scan forever, but we use timed scans).
#define BLE_SCAN_DURATION_SEC     10

// Interval between reconnection attempts (milliseconds).
#define BLE_RECONNECT_INTERVAL_MS 5000

// Keep-alive ping interval to the camera (milliseconds).
// DJI cameras may drop idle BLE links after ~30 s.
#define BLE_KEEPALIVE_INTERVAL_MS 15000

// BLE connection timeout (milliseconds).
#define BLE_CONNECT_TIMEOUT_MS    10000

// ──────────────────────────────────────────────────────────────────────────────
// Status LED (optional, for visual feedback)
// ──────────────────────────────────────────────────────────────────────────────

// Built-in LED on most ESP32-C3 dev boards (GPIO8, active LOW on DevKitM-1).
#define STATUS_LED_PIN        8
#define LED_ACTIVE_LOW        true   // Set to false if your LED is active HIGH

// ──────────────────────────────────────────────────────────────────────────────
// Debug
// ──────────────────────────────────────────────────────────────────────────────

// Uncomment to enable verbose serial debug output on Serial (USB CDC).
#define DEBUG_ENABLED

#ifdef DEBUG_ENABLED
    #define DBG(fmt, ...)   log_printf("[%lu] " fmt "\n", millis(), ##__VA_ARGS__)
#else
    #define DBG(fmt, ...)   ((void)0)
#endif

#endif // CONFIG_H
