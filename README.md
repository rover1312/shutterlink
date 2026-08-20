# shutterlink
Open-source CamLink alternative: an ESP32-C3 BLE bridge that turns a radio switch into record control for DJI Osmo Action cameras and puts battery + recording telemetry into the Betaflight OSD.

> ⚠️ **Requires Betaflight 25.12 or newer.** Betaflight 4.x and 4.5 are
> **not** supported (they lack the Custom Message OSD elements this project
> uses).
> 
> ⚠️ **Not affiliated with or endorsed by DJI or itsFPV.**
> All protocols based on public community reverse-engineering research.
---

## ✨ Features

- 🎛️ **RC-switch record control** — start/stop recording from any AUX channel
  on your transmitter, read straight from Betaflight via MSP.
- 📺 **Live OSD telemetry** — camera status (`REC 85% 00:12:34` / `STBY` /
  `CAM: OFF`) injected into the Betaflight HD OSD via `MSP2_COMMON_SET_TEXT`.
- 📶 **Robust BLE link** — continuous scanning, auto-reconnect, keep-alive
  pings, and a non-blocking state machine (scan → connect → auth → ready).
- 💡 **Status LED patterns** — know your link state at a glance on the bench.
- 🧩 **Modular firmware** — clean separation: `msp_protocol` (FC side),
  `dji_ble_client` (camera side), `main` (state machine).
- 🔧 **Fully configurable** — one header (`config.h`) for pins, channels,
  thresholds, and timings.

---

## 🧠 How It Works

```
 ┌────────────┐   RC frames   ┌────────────────┐  MSP (UART)  ┌─────────────┐
 │  Radio TX  │ ────────────► │ Flight Controller│ ◄──────────► │  ESP32-C3   │
 └────────────                │   (Betaflight)   │   Serial1   │ (ShutterLink)│
                              └────────────────┘              └──────┬──────┘
                                       │                             │
                                       ▼                     BLE (DUML over GATT)
                              ┌────────────────┐                     │
                              │  HD OSD in your│ ◄─ telemetry ──┐    ▼
                              │     goggles    │                │ ┌─────────────┐
                              └────────────────                └─┤ DJI Action 2 │
                                       ▲                          │  (Osmo BLE) │
                                       └── switch state ──────────┤             │
                                                                  └─────────────┘
```

1. **Switch → ESP32:** The firmware polls `MSP_RC` (v1) every 200 ms and
   watches your configured AUX channel (debounced, 1500 µs threshold).
2. **ESP32 → Camera:** On a clean OFF→ON edge it sends the DUML *start record*
   command over BLE (service `0xFFF0`, write-without-response to `0xFFF5`);
   ON→OFF sends *stop record*.
3. **Camera → OSD:** Telemetry is formatted into a 16-char string and pushed
   to the FC with `MSP2_COMMON_SET_TEXT` (MSP v2, `0x203A`) once per second.

---

## 🛒 Hardware Requirements

| Item | Notes |
|---|---|
| **ESP32-C3** board | DevKitM-1 / "Super Mini" class. BLE 5.0, NimBLE stack. |
| **Flight controller** | Running **Betaflight 4.4+** (MSP API ≥ 1.46) with one free UART. |
| **DJI Action 2** | Other Osmo Action models *expected* to work; see Roadmap. |
| Wiring | 4 wires: 5 V, GND, FC TX, FC RX. |

### Wiring

| ESP32-C3 | Flight Controller |
|---|---|
| GPIO20 (RX) | UART **TX** |
| GPIO21 (TX) | UART **RX** |
| 5 V | 5 V |
| GND | GND |

---

## ⚙️ Betaflight Setup

1. **Ports tab:** enable **MSP** on the UART wired to the ESP32 (115200 baud).
2. **OSD tab:** place the text element you're overriding (see
   `OSD_TEXT_TYPE` in `config.h`) where you want the camera status.
3. **Modes tab:** assign any AUX channel as your *record switch* and note its
   channel index (AUX1 = ch5 = index 4 …). Set `AUX_CHANNEL_INDEX` in
   `config.h` to match (default: `8` (AUX 5)).

---

## 🚀 Build & Flash

```bash
# 1. Clone
git clone https://github.com/<you>/shutterlink.git
cd shutterlink

# 2. Build
pio run

# 3. Flash
pio run -t upload

# 4. Watch it work
pio device monitor -b 115200
```

Dependencies (handled by PlatformIO): `h2zero/NimBLE-Arduino @ ^1.4.1`.

---

## 🔧 Configuration (`src/config.h`)

| Define | Default | Purpose |
|---|---|---|
| `FC_UART_RX_PIN` / `FC_UART_TX_PIN` | 20 / 21 | UART pins to the FC |
| `AUX_CHANNEL_INDEX` | 8 | RC channel used as record switch |
| `RC_SWITCH_THRESHOLD` | 1500 | µs above = ON |
| `RC_DEBOUNCE_MS` | 300 | Switch debounce |
| `OSD_TEXT_ROW` / `OSD_TEXT_COL` | 14 / 1 | OSD position |
| `OSD_TEXT_TYPE` | 4 | Which text element to override |
| `BLE_RECONNECT_INTERVAL_MS` | 5000 | Retry cadence when disconnected |
| `BLE_KEEPALIVE_INTERVAL_MS` | 15000 | Link keep-alive ping |
| `STATUS_LED_PIN` | 8 | Onboard LED |

---

## 💡 Status LED Cheat-Sheet

| Pattern | Meaning |
|---|---|
| Slow blink (1 s) | Disconnected / scanning |
| Fast blink (200 ms) | Connecting / authenticating |
| Solid | Connected, camera standby |
| Medium blink (500 ms) | Connected, **recording** |

---

## 📁 Repository Structure

```
shutterlink/
├── platformio.ini          # ESP32-C3 build config + NimBLE dependency
├── README.md
├── LICENSE                 # MIT
└── src/
    ├── config.h            # Pins, channels, timings, debug switch
    ├── main.cpp            # Non-blocking loop: poll → debounce → act → OSD
    ├── msp_protocol.h/.cpp # MSP v1 parser + v2 SET_TEXT builder (CRC-DVB-S2)
    └── dji_ble_client.h/.cpp # NimBLE scan/connect, DUML framing, record cmds
```

---

## 📡 Protocol Notes (for developers)

### FC side — MSP
- **v1** (`$M<`) for `MSP_RC` polling (XOR checksum).
- **v2** (`$X<`) for `MSP2_COMMON_SET_TEXT` (`0x203A`) with CRC-DVB-S2.

### Camera side — DUML over BLE
DJI Osmo cameras speak a proprietary DUML frame over GATT
(service `0xFFF0`; commands → `0xFFF5` *write-without-response*;
notifications → `0xFFF4`):

```
[0x55][len_lo][(ver<<2|len_hi)][crc8][sender][receiver]
[msg_id BE][flags][cmdSet][cmdId][payload…][crc16 LE]
```

Pairing is app-level (`0x07/0x45` SetPairingPIN), **not** OS Bluetooth bonding.
Frame format and CRC parameters follow the community references below.

---

## 🗺️ Roadmap

- [ ] Full telemetry parse (battery %, rec time) from DUML notifications
- [ ] Ghost-display spoofer firmware (ATtiny / RP2040)
- [ ] Auto-approve pairing hook (BLE `0x02` response → injected touch)
- [ ] GoPro / Insta360 profiles
- [ ] BLE OTA firmware updates

---

## 🐛 Troubleshooting

| Symptom | Fix |
|---|---|
| `Connection failed; status=520` on first attempt | Normal — the firmware retries automatically and usually connects on attempt 2. |
| Stuck at `BLE=3` (authenticating) / auth timeout | Camera requires on-screen pairing approval. Approve on the display, or wait for ghost-display support. |
| `Camera not ready — cannot start recording` | BLE not connected/authenticated yet; check the LED pattern and serial log. |
| No OSD text | Verify Betaflight ≥ 4.4, MSP enabled on the UART, and the overridden element placed in the OSD tab. |
| No BLE logs at all | Open the monitor *after* flashing and reset the board; the scan now runs continuously. |

---

## 🙏 Credits & References

- [yigitkonur/lib-osmo-ble](https://github.com/yigitkonur/lib-osmo-ble) — DUML-over-BLE wire format & pairing flow
- [KonradIT/osmosis](https://github.com/KonradIT/osmosis) — hardware-verified Osmo protocol map
- [rhoenschrat/DJI-Remote](https://github.com/rhoenschrat/DJI-Remote) — multicam BLE remote reference
- [dji-sdk/Osmo-GPS-Controller-Demo](https://github.com/dji-sdk/Osmo-GPS-Controller-Demo) — official ESP32 BLE demo
- (https://itsfpv.de/en-int/products/camlink) — the commercial product this open-source project replicates

---

## 📜 License

MIT — do what you want, fly safe, and land your protocols responsibly. 🛸
