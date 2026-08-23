// ============================================================================
// fc_status.h — Flight Controller state polling via MSP
// ============================================================================
// Polls MSP_STATUS / MSP_BOXIDS / MSP_ANALOG plus one-shot identity queries.
//
//   • Armed state      : MSP_STATUS.flightModeFlags bit at the index of the
//                        ARM box ID inside the MSP_BOXIDS array (box 0).
//   • FC battery       : MSP_ANALOG (vbat, rssi)
//   • Identity         : MSP_API_VERSION / FC_VARIANT / FC_VERSION / BOARD_INFO
// ============================================================================

#ifndef FC_STATUS_H
#define FC_STATUS_H

#include <Arduino.h>
#include "config.h"
#include "msp_protocol.h"

struct FcTelemetry {
    bool     armed;             // True while ARM box is active
    bool     fcAlive;           // Valid MSP traffic seen within last ~2 s
    uint16_t cycleTimeUs;
    uint16_t vbat10;            // Battery voltage ×10 (158 = 15.8 V)
    uint16_t rssi;              // 0–1023 (percentage-style RSSI)
};

/// Initialise the FC status poller (call after mspInit()).
void fcStatusInit();

/// Call from loop(); sends periodic requests and parses responses.
/// Pass every valid MSP message here so it can inspect them.
void fcStatusFeed(const MspMessage &msg);

/// Periodic poller — call from loop().
void fcStatusUpdate();

/// Latest snapshot.
const FcTelemetry& fcGetTelemetry();

/// Identity strings (populated shortly after boot, empty until then).
const char* fcApiVersion();     // "1.46"
const char* fcVariant();        // "BTFL"
const char* fcFirmwareVersion();// "4.5.1"
const char* fcBoardName();      // "SPRACINGH7ZERO" etc.

#endif // FC_STATUS_H
