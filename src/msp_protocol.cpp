// ============================================================================
// msp_protocol.cpp — MSP v1 & v2 implementation
// ============================================================================
// References:
//   MSP v1 spec : http://www.multiwii.com/wiki/index.php?title=Multiwii_Serial_Protocol
//   MSP v2 spec : https://github.com/iNavFlight/inav/wiki/MSP-V2
//   Betaflight  : src/main/msp/msp_protocol_v2_betaflight.h (MSP2_SET_TEXT)
//
// ─── MSP v1 Frame Layout ────────────────────────────────────────────────────
//   Byte 0   : '$'          Preamble
//   Byte 1   : 'M'          Preamble
//   Byte 2   : '<' / '>'    Direction  (< = to FC,  > = from FC)
//   Byte 3   : N            Payload size (0–255)
//   Byte 4   : CMD          Command ID
//   Bytes 5…4+N : Payload   N data bytes
//   Last byte   : CRC       XOR of bytes 3..4+N (size ^ cmd ^ payload bytes)
//
// ─── MSP v2 Frame Layout ────────────────────────────────────────────────────
//   Byte 0     : '$'          Preamble
//   Byte 1     : 'X'          Preamble
//   Byte 2     : '<' / '>'    Direction
//   Byte 3     : FLAG         Flags (usually 0)
//   Bytes 4-5  : CMD          Command ID (uint16, little-endian)
//   Bytes 6-7  : SIZE         Payload size (uint16, little-endian)
//   Bytes 8…7+N : Payload     N data bytes
//   Last byte  : CRC8         CRC-DVB-S2 over bytes 3..7+N
// ============================================================================

#include "msp_protocol.h"

// ──────────────────────────────────────────────────────────────────────────────
// Internal state
// ──────────────────────────────────────────────────────────────────────────────

static HardwareSerial *_fcSerial = nullptr;

// Parser state
static MspParserState _parserState = MSP_IDLE;
static uint8_t  _parserPayloadIdx  = 0;
static uint8_t  _parserCrcAccum    = 0;
static MspMessage _parserMsg;

// ──────────────────────────────────────────────────────────────────────────────
// CRC helpers
// ──────────────────────────────────────────────────────────────────────────────

/// CRC-DVB-S2 used by MSP v2.
/// Polynomial: 0xD5.  Initial value: 0.
static uint8_t crcDvbS2(uint8_t crc, uint8_t byte) {
    crc ^= byte;
    for (uint8_t i = 0; i < 8; i++) {
        if (crc & 0x80) {
            crc = (crc << 1) ^ 0xD5;
        } else {
            crc = crc << 1;
        }
    }
    return crc;
}

/// Compute CRC-DVB-S2 over a buffer.
static uint8_t crcDvbS2Buf(const uint8_t *buf, size_t len) {
    uint8_t crc = 0;
    for (size_t i = 0; i < len; i++) {
        crc = crcDvbS2(crc, buf[i]);
    }
    return crc;
}

// ──────────────────────────────────────────────────────────────────────────────
// Initialisation
// ──────────────────────────────────────────────────────────────────────────────

void mspInit(HardwareSerial &serial) {
    _fcSerial = &serial;
    // Configure Serial1 with the FC UART pins and baud rate.
    _fcSerial->begin(FC_UART_BAUD, SERIAL_8N1, FC_UART_RX_PIN, FC_UART_TX_PIN);
    DBG("MSP: UART1 initialised at %d baud (RX=%d, TX=%d)",
        FC_UART_BAUD, FC_UART_RX_PIN, FC_UART_TX_PIN);
}

// ──────────────────────────────────────────────────────────────────────────────
// MSP v1 Parser — Feed one byte at a time
// ──────────────────────────────────────────────────────────────────────────────

bool mspParseByte(uint8_t byte, MspMessage &outMsg) {
    switch (_parserState) {

        // ── Waiting for '$' ──────────────────────────────────────────────
        case MSP_IDLE:
            if (byte == MSP_V1_HEADER_DOLLAR) {
                _parserState = MSP_HEADER_M;
            }
            break;

        // ── Expecting 'M' (v1) or 'X' (v2 preamble) ─────────────────────
        case MSP_HEADER_M:
            if (byte == MSP_V1_HEADER_M) {
                _parserState = MSP_V1_DIRECTION;
            } else {
                // We only parse v1 responses; skip anything else.
                _parserState = MSP_IDLE;
            }
            break;

        // ── Direction byte ──────────────────────────────────────────────
        case MSP_V1_DIRECTION:
            _parserMsg.isError = (byte == MSP_V1_DIR_ERROR);
            if (byte == MSP_V1_DIR_FROM_FC || byte == MSP_V1_DIR_ERROR) {
                _parserState = MSP_V1_SIZE;
            } else {
                // Unexpected direction — might be an outgoing echo; skip.
                _parserState = MSP_IDLE;
            }
            break;

        // ── Payload size ────────────────────────────────────────────────
        case MSP_V1_SIZE:
            _parserMsg.payloadSize = byte;
            _parserCrcAccum = byte;   // CRC starts with the size byte
            _parserPayloadIdx = 0;
            if (byte > MSP_MAX_PAYLOAD_SIZE) {
                // Payload too large for our buffer — drop frame.
                DBG("MSP: payload too large (%d bytes), dropping", byte);
                _parserState = MSP_IDLE;
            } else {
                _parserState = MSP_V1_CMD;
            }
            break;

        // ── Command ID ──────────────────────────────────────────────────
        case MSP_V1_CMD:
            _parserMsg.cmd = byte;
            _parserCrcAccum ^= byte;  // XOR command into CRC
            if (_parserMsg.payloadSize > 0) {
                _parserState = MSP_V1_PAYLOAD;
            } else {
                _parserState = MSP_V1_CRC;
            }
            break;

        // ── Payload bytes ───────────────────────────────────────────────
        case MSP_V1_PAYLOAD:
            _parserMsg.payload[_parserPayloadIdx++] = byte;
            _parserCrcAccum ^= byte;  // XOR each payload byte
            if (_parserPayloadIdx >= _parserMsg.payloadSize) {
                _parserState = MSP_V1_CRC;
            }
            break;

        // ── CRC verification ────────────────────────────────────────────
        case MSP_V1_CRC:
            _parserState = MSP_IDLE;  // Reset for next frame
            if (_parserCrcAccum == byte) {
                // CRC matches — deliver message.
                _parserMsg.valid = true;
                outMsg = _parserMsg;
                return true;
            } else {
                DBG("MSP: CRC mismatch (expected 0x%02X, got 0x%02X)",
                    _parserCrcAccum, byte);
                _parserMsg.valid = false;
            }
            break;
    }
    return false;
}

// ──────────────────────────────────────────────────────────────────────────────
// MSP v1 — Send a request (no payload)
// ──────────────────────────────────────────────────────────────────────────────
// Frame: $M< [size=0] [cmd] [crc = 0^cmd = cmd]

void mspSendRequest(uint8_t cmdId) {
    if (!_fcSerial) return;

    uint8_t frame[6];
    frame[0] = MSP_V1_HEADER_DOLLAR;  // '$'
    frame[1] = MSP_V1_HEADER_M;       // 'M'
    frame[2] = MSP_V1_DIR_TO_FC;      // '<'
    frame[3] = 0;                      // Payload size = 0
    frame[4] = cmdId;                  // Command ID
    frame[5] = 0 ^ cmdId;             // CRC = size ^ cmd (size is 0)

    _fcSerial->write(frame, 6);
    _fcSerial->flush();  // Ensure the bytes are sent immediately
}

// ──────────────────────────────────────────────────────────────────────────────
// MSP v2 — Generic command sender
// ──────────────────────────────────────────────────────────────────────────────

void mspSendV2Command(uint16_t cmdId, const uint8_t *payload, uint16_t payloadLen) {
    if (!_fcSerial) return;

    // CRC region = flag(1) + cmd(2) + size(2) + payload(payloadLen)
    uint16_t crcRegionLen = 5 + payloadLen;
    uint8_t buf[3 + MSP_MAX_PAYLOAD_SIZE + 6];

    buf[0] = MSP_V2_HEADER_DOLLAR;  // '$'
    buf[1] = MSP_V2_HEADER_X;       // 'X'
    buf[2] = MSP_V2_DIR_TO_FC;      // '<'
    buf[3] = 0x00;                  // Flags

    // Command ID (uint16 LE)
    buf[4] = (uint8_t)(cmdId & 0xFF);
    buf[5] = (uint8_t)((cmdId >> 8) & 0xFF);

    // Payload size (uint16 LE)
    buf[6] = (uint8_t)(payloadLen & 0xFF);
    buf[7] = (uint8_t)((payloadLen >> 8) & 0xFF);

    if (payload && payloadLen) {
        memcpy(&buf[8], payload, payloadLen);
    }

    uint8_t crc = crcDvbS2Buf(&buf[3], crcRegionLen);
    buf[3 + crcRegionLen] = crc;

    _fcSerial->write(buf, 3 + crcRegionLen + 1);
    _fcSerial->flush();
}

// ──────────────────────────────────────────────────────────────────────────────
// MSP v2 — SET_TEXT (0x3007): set pilot/craft/custom OSD text elements
// ──────────────────────────────────────────────────────────────────────────────
// Payload layout (verified against Betaflight master msp.c):
//   Byte 0        : Text type (1=PILOT_NAME … 7..10=CUSTOM_MSG_0..3)
//   Byte 1        : String length (N)
//   Bytes 2..1+N  : String data (ASCII, NOT null-terminated in the packet)

void mspSendSetText(uint8_t textType, const char *text) {
    uint8_t textLen = (uint8_t)strlen(text);
    if (textLen > OSD_MAX_TEXT_LEN) textLen = OSD_MAX_TEXT_LEN;

    uint8_t payload[2 + OSD_MAX_TEXT_LEN];
    payload[0] = textType;
    payload[1] = textLen;
    if (textLen) memcpy(&payload[2], text, textLen);

    mspSendV2Command(MSP2_SET_TEXT, payload, 2 + textLen);
}

// ──────────────────────────────────────────────────────────────────────────────
// Utility — Extract RC channel from MSP_RC response
// ──────────────────────────────────────────────────────────────────────────────

uint16_t mspGetRcChannel(const MspMessage &msg, uint8_t channelIndex) {
    // Each channel is 2 bytes (uint16 LE).  MSP_RC payload = N channels × 2.
    uint8_t offset = channelIndex * 2;
    if (offset + 1 >= msg.payloadSize) {
        return 0;  // Channel index out of range for this response.
    }
    return (uint16_t)msg.payload[offset] | ((uint16_t)msg.payload[offset + 1] << 8);
}
