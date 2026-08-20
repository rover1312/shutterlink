// ============================================================================
// msp_protocol.h — MultiWii Serial Protocol (MSP) v1 & v2 interface
// ============================================================================
// Handles:
//   • Building & sending MSP v1 requests (e.g., MSP_RC)
//   • Parsing MSP v1 responses (variable-length payloads)
//   • Building & sending MSP v2 packets (e.g., MSP2_COMMON_SET_TEXT)
//   • CRC calculations (XOR for v1, CRC-DVB-S2 for v2)
// ============================================================================

#ifndef MSP_PROTOCOL_H
#define MSP_PROTOCOL_H

#include <Arduino.h>
#include "config.h"

// ──────────────────────────────────────────────────────────────────────────────
// MSP Command IDs
// ──────────────────────────────────────────────────────────────────────────────

// MSP v1 commands
#define MSP_RC                105  // Request: no payload. Response: N×uint16 channels.

// MSP v2 commands (16-bit IDs, sent via $X frame)
#define MSP2_COMMON_SET_TEXT  0x203A  // Set OSD text element

// ──────────────────────────────────────────────────────────────────────────────
// MSP Packet Constants
// ──────────────────────────────────────────────────────────────────────────────

// Maximum expected payload size for MSP responses we care about.
// MSP_RC response: up to 18 channels × 2 bytes = 36 bytes.
#define MSP_MAX_PAYLOAD_SIZE  64

// MSP v1 header bytes
#define MSP_V1_HEADER_DOLLAR  '$'
#define MSP_V1_HEADER_M       'M'
#define MSP_V1_DIR_TO_FC      '<'   // Outgoing (to Flight Controller)
#define MSP_V1_DIR_FROM_FC    '>'   // Incoming (from Flight Controller)
#define MSP_V1_DIR_ERROR      '!'   // Error response

// MSP v2 header bytes
#define MSP_V2_HEADER_DOLLAR  '$'
#define MSP_V2_HEADER_X       'X'
#define MSP_V2_DIR_TO_FC      '<'
#define MSP_V2_DIR_FROM_FC    '>'

// ──────────────────────────────────────────────────────────────────────────────
// MSP Parser State Machine
// ──────────────────────────────────────────────────────────────────────────────

enum MspParserState : uint8_t {
    MSP_IDLE,           // Waiting for '$'
    MSP_HEADER_M,       // Got '$', expecting 'M' (v1) or 'X' (v2)
    MSP_V1_DIRECTION,   // Got 'M', expecting direction char
    MSP_V1_SIZE,        // Expecting payload size byte
    MSP_V1_CMD,         // Expecting command byte
    MSP_V1_PAYLOAD,     // Receiving payload bytes
    MSP_V1_CRC,         // Expecting CRC byte (XOR checksum)
    // (MSP v2 parsing is not needed for responses in this project,
    //  because the FC only responds to MSP_RC via v1. But we include
    //  the v2 *builder* for outgoing SET_TEXT packets.)
};

// ──────────────────────────────────────────────────────────────────────────────
// Parsed MSP Message
// ──────────────────────────────────────────────────────────────────────────────

struct MspMessage {
    uint8_t  cmd;                          // Command ID
    uint8_t  payloadSize;                  // Number of payload bytes
    uint8_t  payload[MSP_MAX_PAYLOAD_SIZE]; // Payload buffer
    bool     isError;                      // True if direction was '!'
    bool     valid;                        // True if CRC check passed
};

// ──────────────────────────────────────────────────────────────────────────────
// Public API
// ──────────────────────────────────────────────────────────────────────────────

/// Initialise MSP layer: configures Serial1 for FC communication.
void mspInit(HardwareSerial &serial);

/// Feed one incoming byte into the MSP v1 parser.
/// Returns true when a complete, valid message is available in `outMsg`.
bool mspParseByte(uint8_t byte, MspMessage &outMsg);

/// Send an MSP v1 request with no payload (e.g., MSP_RC poll).
void mspSendRequest(uint8_t cmdId);

/// Send an MSP v2 SET_TEXT packet to push OSD text to the FC.
///   textType : 0=PILOT_NAME, 1=CRAFT_NAME, etc.
///   row, col : position on the HD OSD grid.
///   text     : null-terminated string to display.
void mspSendOSDText(uint8_t textType, uint8_t row, uint8_t col,
                    const char *text);

/// Extract a single 16-bit unsigned RC channel value from an MSP_RC payload.
///   channelIndex : 0-based index (0 = Roll, 1 = Pitch, …).
///   msg          : a valid MSP_RC response message.
/// Returns the channel value in µs (typically 1000–2000), or 0 on error.
uint16_t mspGetRcChannel(const MspMessage &msg, uint8_t channelIndex);

#endif // MSP_PROTOCOL_H
