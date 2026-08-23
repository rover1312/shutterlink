// ============================================================================
// recorder.h — Recording decision engine
// ============================================================================
// Combines three inputs into one "recording desired" signal and drives the
// camera commands on transitions:
//
//   • RC record switch (debounced, threshold from settings)
//   • Record-on-arm (FC arm state from MSP_STATUS, optional stop-on-disarm)
//   • Manual start/stop from the Web UI
//
// Camera commands are absolute ("start"/"stop"), so re-sending after a BLE
// reconnect is always safe — never a toggle.
// ============================================================================

#ifndef RECORDER_H
#define RECORDER_H

#include <Arduino.h>

/// Initialise the recorder (call from setup()).
void recorderInit();

/// Feed the latest raw RC channel value in µs (from MSP_RC responses).
void recorderFeedRcValue(uint16_t rcValueUsec);

/// Call from loop(). Evaluates desired state and sends commands on edges.
void recorderUpdate();

/// Manual control from the Web UI.
void recorderManualStart();
void recorderManualStop();

/// Current inputs/state (for LED patterns, OSD, Web UI).
bool recorderSwitchOn();          // Debounced RC switch state
bool recorderDesiredRecording();  // Effective "should be recording" flag
uint16_t recorderLastRcValue();   // Last raw channel value (µs)

#endif // RECORDER_H
