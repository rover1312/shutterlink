# OTA Test Package — ShutterLink `cleanup` @ `3467d73`, version `v2.1`

I could not compile a `.bin` in this session: this machine has no toolchain
(no `python`/`pio`/`arduino-cli`/`java`/`gcc` — verified via `Get-Command`).
So there is **no firmware binary** in this folder by design. Faking one would
brick your board. Instead this folder gives you a one-command build + safe
OTA test flow for a machine that has PlatformIO.

## 1. Build (on a machine with PlatformIO)
```powershell
cd C:\Users\risha\OneDrive\Documents\shutterlink
pio run -e esp32c3
# Output: .pio\build\esp32c3\firmware.bin
Copy-Item .pio\build\esp32c3\firmware.bin .\ota-test\ -Force
```
Or run `.\ota-test\build-ota.ps1` (does the same + writes manifest + size check).
Expect: app partition `min_spiffs.csv`, NimBLE 1.4.1, `FIRMWARE_VERSION v2.1`
(`src/config.h:17`). Note binary size vs `ESP.getFreeSketchSpace()` — OTA
rejects oversize images (`src/web_server.cpp:758`).

## 2. Pre-flight (do not skip)
- USB-flash once first so you have a known-good fallback: `pio run -e esp32c3 -t upload`.
- Record current version: open `http://192.168.4.1/api/ota/status` → `{"version":"v2.1",...}`.
- Join AP `ShutterLink`, keep USB serial open at 115200 for `OTA Start/Success` logs.
- Keep power stable (OTA + WiFi + BLE peaks brownout on weak 5V leads).
- Close dashboard tabs except the uploader (single 2.4GHz radio).

## 3. Upload via Web UI
FC/System tab → Firmware Update → select `ota-test\firmware.bin` → Upload.
What the `cleanup` code does (`src/web_server.cpp:732-804,913`):
- Streams via `handleOtaUpload`, single final JSON from `handleOtaResponse`.
- Aborts on begin/write/end failure (`Update.abort()`, no half-flash boot).
- Reboots only on verified `Update.end(true)`.
- Watch serial: `OTA Start: firmware.bin` → `OTA Success: N bytes` → reboot.

## 4. Verify
- `GET /api/ota/status` version + `uptime` resets, heap sane.
- Dashboard loads (67KB PROGMEM UI), BLE reconnects to saved camera, MSP RC flows.
- Failure signs: `{"ok":false,"error":"..."}` (too large / begin / write / end), serial `OTA Write/End Error`, or boot loop → re-flash over USB, then report the serial lines + `api/ota/status` output.

## 5. Rollback
OTA keeps no A/B slot here. Rollback = USB re-flash previous `firmware.bin`:
`pio run -e esp32c3 -t upload` from `main`/`bce7aff`.
