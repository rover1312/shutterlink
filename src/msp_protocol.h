// ============================================================================
// msp_protocol.h — MultiWii Serial Protocol (MSP) v1 & v2 interface
// ============================================================================
// Handles:
//   • Building & sending MSP v1 requests (e.g., MSP_RC, MSP_STATUS)
//   • Parsing MSP v1 responses (variable-length payloads)
//   • Building & sending MSP v2 packets (MSP2_SET_TEXT for custom messages)
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
#define MSP_API_VERSION       1    // Resp: protocolVer u8, apiMajor u8, apiMinor u8
#define MSP_FC_VARIANT        2    // Resp: 4-char identifier ("BTFL")
#define MSP_FC_VERSION        3    // Resp: major u8, minor u8, patch u8
#define MSP_BOARD_INFO        4    // Resp: identifier[4], revision u16, ...
#define MSP_STATUS            101  // Resp: cycleTime u16, i2cErrors u16,
                                   //       sensors u16, flightModeFlags u32,
                                   //       pidProfile u8
#define MSP_ANALOG            110  // Resp: vbat u8 (0.1V), mAhDrawn u16,
                                   //       rssi u16, amperage i16
#define MSP_BOXIDS            119  // Resp: array of active box IDs
#define MSP_RC                105  // Request: no payload. Response: N×uint16 channels.

// MSP v2 commands (16-bit IDs, sent via $X frame)
#define MSP2_SET_TEXT         0x3007  // Set OSD text element (pilot/craft/custom)

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

/// Send a raw MSP v2 command packet ($X<) with arbitrary payload.
void mspSendV2Command(uint16_t cmdId, const uint8_t *payload, uint16_t payloadLen);

/// Send an MSP v2 SET_TEXT packet (0x3007) to set an OSD text element.
///   textType : 1=PILOT_NAME, 2=CRAFT_NAME, … 7..10 = CUSTOM_MSG_0..3
///   text     : null-terminated string (max OSD_MAX_TEXT_LEN chars).
/// Payload layout: [type u8][len u8][chars…]
void mspSendSetText(uint8_t textType, const char *text);

/// Extract a single 16-bit unsigned RC channel value from an MSP_RC payload.
uint16_t mspGetRcChannel(const MspMessage &msg, uint8_t channelIndex);

// Little-endian payload readers ------------------------------------------------

inline uint16_t mspReadU16(const MspMessage &msg, uint8_t offset) {
    if (offset + 1 >= msg.payloadSize) return 0;
    return (uint16_t)msg.payload[offset] | ((uint16_t)msg.payload[offset + 1] << 8);
}

inline uint32_t mspReadU32(const MspMessage &msg, uint8_t offset) {
    if (offset + 3 >= msg.payloadSize) return 0;
    return (uint32_t)msg.payload[offset] |
           ((uint32_t)msg.payload[offset + 1] << 8) |
           ((uint32_t)msg.payload[offset + 2] << 16) |
           ((uint32_t)msg.payload[offset + 3] << 24);
}

#endif // MSP_PROTOCOL_H
