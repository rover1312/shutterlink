// ============================================================================
// web_server.h — SoftAP + captive portal + REST API
// ============================================================================
// Connect a phone/PC to the ShutterLink Wi-Fi network and browse to
// http://192.168.4.1 (captive portal pops up automatically on most devices).
// ============================================================================

#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>

/// Bring up the SoftAP (credentials from settings), DNS captive portal and
/// HTTP routes.
void webInit();

/// Power the Wi-Fi radio down completely (AP + DNS + HTTP off).
/// BLE / camera control is unaffected.  webStart() can bring it back.
void webStop();

/// Alias of webInit() — used by the radio Wi-Fi switch to re-enable the AP.
void webStart();

/// Call from loop(). Services DNS + HTTP clients (non-blocking-ish).
void webUpdate();

/// Current AP IP as string ("192.168.4.1").
const char* webApIp();

/// True once Wi-Fi/DNS/HTTP are up.
bool webIsUp();

#endif // WEB_SERVER_H
