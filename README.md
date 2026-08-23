# shutterlink

Open-source CamLink alternative: an ESP32-C3 BLE bridge that turns a radio
switch (or your arming switch!) into record control for **DJI Osmo Action**
and **GoPro HERO8+** cameras, and pushes live camera telemetry into the
Betaflight OSD - with a built-in **Glassmorphism Web UI**.

> **Requires a Betaflight build with Custom Message OSD elements**
> (`MSP2_SET_TEXT` 0x3007, custom message types 7-10). Betaflight 4.x and
> 4.5 are **not** supported.
>
> **Not affiliated with or endorsed by DJI, GoPro or itsFPV.**
> All protocols based on public documentation / community reverse-engineering.

---

## Features

- **RC-switch record control** - start/stop recording from *any* AUX channel,
  read straight from Betaflight via MSP. Channel, threshold and debounce are
  configurable from the Web UI - no recompiling.
- **Record-on-arm** - optional auto-start when the FC arms, with optional
  stop-on-disarm. Toggle it in the Web UI.
- **Parallel OSD telemetry on all 4 Custom Messages** - assign any of
  Cam status / Rec time / Battery / Link state / FC battery / Arm state / Off
  to each slot via the Web UI. Pushed with `MSP2_SET_TEXT` (MSP v2, `0x3007`).
- **Two camera backends** - DJI Osmo Action (DUML over BLE) and GoPro HERO8
  through HERO13 (official Open GoPro BLE API), switchable at runtime.
- **Robust BLE link** - continuous scanning, auto-reconnect, keep-alive,
  non-blocking state machine. Camera commands are absolute start/stop (never
  toggles), so retries after reconnects are always safe.
- **Built-in Web UI** - connect to the ESP32's Wi-Fi network and a modern
  Glassmorphism dashboard opens automatically (captive portal): live status,
  manual REC/STOP buttons, all configuration, dark & light mode, frosted-glass
  SVG icon set.
- **Persistent settings** - everything you configure lives in NVS flash.
- **Status LED patterns** - know your link state at a glance on the bench.
- **Modular firmware** - `msp_protocol` (FC side), `fc_status` (arming),
  `dji_camera` / `gopro_camera` (camera side), `camera_manager` (dispatch),
  `recorder` (decision engine), `osd_slots` (OSD), `web_server` + `web_assets`
  (UI).

---

## How It Works

```
 +-----------+  RC frames   +------------------+ MSP (UART) +---------------+
 | Radio TX  | -----------> | Flight Controller| <--------> |   ESP32-C3    |
 +-----------+              |   (Betaflight)   |  Serial1   | (ShutterLink) |
                            +------------------+            +-------+-------+
                                     ^                              | BLE
                                     |                              +-> DJI DUML
                          HD OSD custom                             +-> GoPro Open
                          messages 1-4 <-- telemetry --+                BLE API
                                                                       |
                 phone/PC  <-- Wi-Fi AP + captive portal --------------+
                                  (Web UI)
```

1. **Switch / arm to ESP32:** polls `MSP_RC` every 200 ms; watches your
   configured AUX channel (debounced, configurable threshold). Polls
   `MSP_STATUS` + `MSP_BOXIDS` for arming state.
2. **ESP32 to Camera:** desired-recording = switch ON OR (record-on-arm AND
   armed). On transitions it sends the camera's absolute start/stop command
   over BLE. Manual buttons in the Web UI do the same.
3. **Camera to OSD:** up to four independent strings pushed on change (checked
   every 500 ms) into Betaflight Custom Messages 1-4 via `MSP2_SET_TEXT`.
4. **Web UI:** the ESP32 runs a SoftAP (default SSID `ShutterLink`, password
   `shutterlink`). Browse to `http://192.168.4.1`.

---

## Hardware Requirements

| Item | Notes |
|---|---|
| **ESP32-C3** board | DevKitM-1 / "Super Mini" class. BLE 5.0, NimBLE stack. |
| **Flight controller** | Betaflight with Custom Message OSD elements + one free UART. |
| **Camera** | DJI Osmo Action family **or** GoPro HERO8/9/10/11/12/13. |
| Wiring | 4 wires: 5 V, GND, FC TX, FC RX. |

### Wiring

| ESP32-C3 | Flight Controller |
|---|---|
| GPIO20 (RX) | UART **TX** |
| GPIO21 (TX) | UART **RX** |
| 5 V | 5 V |
| GND | GND |

---

## Betaflight Setup

1. **Ports tab:** enable **MSP** on the UART wired to the ESP32 (115200 baud).
2. **OSD tab:** place **Custom Message 1-4** elements wherever you want them -
   position comes from Betaflight, content from ShutterLink.
3. Assign an AUX channel as your record switch if you use switch control.

---

## Web UI

Connect a phone/PC to the ShutterLink Wi-Fi network - the captive portal opens
automatically on most devices, otherwise browse to `http://192.168.4.1`.

| Tab | What you can do |
|---|---|
| **Dashboard** | Live link/camera/FC status, big START / STOP buttons, live preview of the four OSD strings. |
| **Controls** | Record switch channel (CH5-16/AUX), ON threshold, debounce, **record-on-arm + stop-on-disarm toggles**, Wi-Fi AP credentials. |
| **Camera** | Switch DJI Osmo / GoPro at runtime, pairing instructions, reboot. |
| **OSD** | Assign content to Custom Message slots 1-4 with live previews. |
| **FC / System** | Betaflight identity (API/firmware/board), battery, arm state, heap/uptime, reboot. |

The UI is a single-page app embedded in the firmware (PROGMEM, ~33 KB):
frosted glass cards, backdrop blur, animated gradient background, smooth
transitions, and a dark/light mode toggle that persists in your browser.

### Can it configure Betaflight from the Web UI?

A full Configurator port is not realistic on an ESP32-C3 (the real
Configurator is a multi-megabyte desktop-class app and needs a serial/WebSocket
bridge). But the firmware already speaks MSP in both directions over the FC
UART, so a lightweight config panel - reading/writing selected settings via
MSP passthrough (think "Betaflight Lua scripts in a browser") - is absolutely
feasible as a future subtab. The FC/System tab already demonstrates live MSP
data flowing from your FC.

---

## Camera Support Notes

### DJI Osmo Action (DUML over BLE)

Service `0xFFF0`; DUML frames written without response to `0xFFF5`,
notifications on `0xFFF4`. App-level pairing (`0x07/0x45`), not OS bonding -
approve prompts on the camera screen when they appear.

```
[0x55][len_lo][(ver<<2|len_hi)][crc8][sender][receiver]
[msg_id BE][flags][cmdSet][cmdId][payload...][crc16 LE]
```

Record = cmdSet `0x0A`, cmdId `0x0D`, payload `0x01` start / `0x00` stop.

### GoPro HERO8+ (Open GoPro BLE)

Official public API: service `0xFEA6`, command characteristic
`b5f90072-aa8d-11e3-9046-0002a5d5c51b`.

- Start:  `03 1A {%230%22shutter%22%3Atrue}`
- Stop:   `03 1B {%230%22shutter%22%3Afalse}`
- Keep-alive every ~3 s: `02 01 42`
- Status registration for battery % + encoding state.

First connection: put the camera into pairing mode and approve the prompt on
its screen once; the ESP32 bonds and reconnects silently afterwards.
HERO5-7 use a different legacy protocol and are not supported.

---

## Build & Flash

```bash
# 1. Clone
git clone https://github.com/rover1312/shutterlink.git
cd shutterlink

# 2. Build
pio run

# 3. Flash
pio run -t upload

# 4. Watch it work
pio device monitor -b 115200
```

Dependencies (handled by PlatformIO): `h2zero/NimBLE-Arduino @ ^1.4.1`.
Partition table switched to `min_spiffs.csv` (BLE + Wi-Fi + web UI need the
bigger app slot).

---

## Configuration

Runtime settings live in NVS and are edited from the Web UI. Compile-time
defaults are in `src/config.h`:

| Define | Default | Purpose |
|---|---|---|
| `FC_UART_RX_PIN` / `FC_UART_TX_PIN` | 20 / 21 | UART pins to the FC |
| `DEFAULT_CAMERA_TYPE` | DJI | Initial camera backend |
| `DEFAULT_AUX_CHANNEL_INDEX` | 8 | RC channel used as record switch |
| `DEFAULT_RC_THRESHOLD_US` | 1500 | us above = ON |
| `DEFAULT_RC_DEBOUNCE_MS` | 300 | Switch debounce |
| `DEFAULT_RECORD_ON_ARM` | false | Auto-record on arming |
| `WIFI_AP_DEFAULT_SSID` / `_PASS` | ShutterLink / shutterlink | Web UI hotspot |
| `DEFAULT_OSD_SLOT_1..4` | status/time/batt/link | Custom Message contents |
| `STATUS_LED_PIN` | 8 | Onboard LED |

## Repository Structure

```
shutterlink/
+-- platformio.ini          # ESP32-C3 build config + NimBLE dependency
+-- README.md
+-- src/
    +-- config.h            # Pins, defaults, timings, debug switch
    +-- settings.h/.cpp     # NVS-backed runtime configuration
    +-- main.cpp            # Non-blocking loop orchestration
    +-- msp_protocol.h/.cpp # MSP v1 parser + MSP v2 SET_TEXT (CRC-DVB-S2)
    +-- fc_status.h/.cpp    # Arm detection, FC battery/identity polling
    +-- camera_common.h     # Shared camera types
    +-- dji_camera.h/.cpp   # DJI Osmo DUML-over-BLE backend
    +-- gopro_camera.h/.cpp # GoPro Open BLE backend
    +-- camera_manager.*    # Backend dispatcher (runtime switching)
    +-- recorder.h/.cpp     # Switch/arm/manual -> record decision engine
    +-- osd_slots.h/.cpp    # Custom Message 1-4 content manager
    +-- web_server.h/.cpp   # SoftAP, captive DNS, REST API
    +-- web_assets.h        # Embedded Glassmorphism Web UI (PROGMEM)
```

## Status LED Cheat-Sheet

| Pattern | Meaning |
|---|---|
| Slow blink (1 s) | Disconnected / scanning |
| Fast blink (200 ms) | Connecting / authenticating |
| Solid | Connected, camera standby |
| Medium blink (500 ms) | Recording desired |

## Known Limitations (ESP32-C3)

- Wi-Fi and BLE share one radio; heavy Wi-Fi traffic can slightly delay BLE.
  Normal dashboard usage is no problem.
- GoPro telemetry depth depends on model firmware (battery %, encoding state;
  record timer is counted locally while encoding).
- DJI telemetry parsing beyond link state is still experimental upstream.

## Roadmap

- [x] GoPro profile (Open GoPro BLE)
- [x] Parallel info on all four custom messages
- [x] Glassmorphism Web UI with dark/light mode
- [x] Record-on-arm (+ stop-on-disarm)
- [ ] Lightweight MSP config panel ("configurator-lite" subtab)
- [ ] Profiles , Camera configuration.
- [ ] Full DJI telemetry parse (battery %, rec time from DUML notifications)


## Credits & References

- [yigitkonur/lib-osmo-ble](https://github.com/yigitkonur/lib-osmo-ble) - DUML-over-BLE wire format & pairing flow
- [KonradIT/osmosis](https://github.com/KonradIT/osmosis) - hardware-verified Osmo protocol map
- [gopro/OpenGoPro](https://github.com/gopro/OpenGoPro) - official Open GoPro BLE specification
- [rhoenschrat/DJI-Remote](https://github.com/rhoenschrat/DJI-Remote) - multicam BLE remote reference
- [Easy4Racing/bf_custom_osd_msg_example](https://github.com/Easy4Racing/bf_custom_osd_msg_example) - BF custom message reference
- [betaflight/betaflight](https://github.com/betaflight/betaflight) - MSP protocol source of truth
- [itsfpv CamLink](https://itsfpv.de/en-int/products/camlink) - the commercial product this project replicates

## License

MIT - do what you want, fly safe, and land your protocols responsibly.

