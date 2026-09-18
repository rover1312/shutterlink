# ShutterLink Firmware Audit — Issue Tracker (`cleanup` branch @ `aeebeb4`)

> Living document. Each finding from the Senior Embedded / Security audit is tracked here.
> Status values: `OPEN` (acknowledged, not fixed) | `FIXED` (fixed in `cleanup`, commit ref) | `REASONED` (kept by design — rationale below, no fix).
> Recheck method for each: code comment + `git log/blame` + protocol spec (DUML/OpenGoPro/Betaflight/MSP) + hardware constraint (C3 single-core, no PSRAM, single 2.4GHz radio).

Branch: `cleanup` (base `bce7aff`). Target: `esp32-c3-devkitm-1`, Arduino + NimBLE 1.4.1.
Scope note: repo is **not** ESP32-CAM video streaming (no `esp_camera`, MJPEG/RTSP/WebRTC, DMA, PSRAM framebuffers, XCLK). Video-specific prompt items are marked N/A with mapped equivalent.

## Status legend
- `FIXED(cleanup)` = already fixed in unpushed/current `cleanup` edits or in this pass (see Fix commit column).
- `REASONED` = investigated, has backing (spec/hardware/UX/safety), kept. Rationale must cite evidence.
- `OPEN` = no backing found yet or fix deferred (roadmap). Needs follow-up.

## Master table

| ID | Domain | Severity | File:Line (`cleanup`) | Title | Status | Fix commit / Reason ref |
|----|--------|----------|------------------------|-------|--------|--------------------------|
| E01 | 1 Embedded | HIGH | `src/web_server.cpp:110,587` | 2.2KB+4KB static JSON buffers resident forever | REASONED | R-E01: C3 no PSRAM, stack ~8KB; static avoids stack overflow + no alloc in request path. Size matches 10-entry worst case. Future: chunked send |
| E02 | 1 Embedded | HIGH | `src/web_server.cpp:312,437,461,612` | `String arg("plain")` heap duplication per POST | OPEN | Needs ArduinoJson StaticDoc; 2KB cap mitigates DoS, fragmentation remains |
| E03 | 1 Embedded | MEDIUM | `src/dji_camera.cpp:118-121,133-138` / `src/gopro_camera.cpp:152-171` | `std::string` alloc in NimBLE host-task callback | REASONED | R-E03: NimBLE API returns `std::string` temporaries; copy-then-use is required to avoid dangle. Small (<32B), short-lived. Alternative (raw addr bytes) adds complexity for marginal gain |
| E04 | 1 Embedded | HIGH | `src/dji_camera.cpp:480-500` / `src/gopro_camera.cpp:455-475` | `DBG`+`snprintf`+`writeValue` in notify (host task) | OPEN | Violates own no-block rule; needs flag-defer to loop(). Not fixed — needs refactor |
| E05 | 1 Embedded | MEDIUM | `src/camera_manager.cpp:49-78` | Partial mutex coverage, `shutdownActiveBackend` unlocked | FIXED(cleanup) | `aeebeb4`: 50ms timeout + documented single-thread ownership. Full removal deferred |
| E06 | 1 Embedded | MEDIUM | `src/main.cpp:119-146` | UART pump + per-byte `settingsGet()` | OPEN | Budget (64B) fixed in `aeebeb4`; per-call cache micro-opt left open |
| E07 | 1 Embedded | HIGH | `src/web_server.cpp:615-680` | `handleMspPost` blocks loop 500ms + steals UART | OPEN | Design was single-thread simplicity; no backing for 500ms. Needs async; documented |
| E08 | 1 Embedded | MEDIUM | `src/msp_protocol.cpp:185,222` | `Serial.flush()` per TX blocks | OPEN | Was “ensure bytes sent”; UART FIFO drains async, flush unneeded. Fix: drop flush |
| E09 | 1 Embedded | MEDIUM | `src/settings.cpp:79-103` / `src/cam_registry.cpp:52-70` | Full NVS rewrite per tweak, double persist on pair | OPEN | Simplicity over wear; no dirty-check. Needs fix |
| E10 | 1 Embedded | LOW | `src/web_assets.h:19` / `src/web_server.cpp:103-107` | 67KB `send_P` blocks loop | REASONED | R-E10: single-page PROGMEM UI, `no-store` intentional for rapid iteration. Concurrent `/` rare (1 user). Timeout + close added |
| E11 | 1 Embedded | INFO | `src/config.h:170` / `platformio.ini:27` | `DEBUG_ENABLED`+`CORE_DEBUG_LEVEL=3` always on | REASONED | R-E11: bench-first project, USB logs are primary field-debug tool. Flight profile documented (`-DDEBUG_DISABLED`) |
| E12 | 1 Embedded | LOW | `src/scan_results.cpp:14` / `src/cam_registry.cpp:26-34` | Dual discovered tables + lossy `_pending` funnel | OPEN | Legacy funnel kept for compat; marked LEGACY. Needs deletion |
| E13 | 1 Embedded | LOW | `src/msp_protocol.h:76-82` | `MspMessage msg;` uninit | FIXED(this pass) | Zero-init `MspMessage msg{}` — no reason for uninit, fixed |
| V01 | 2 Camera proto | CRITICAL | `src/dji_camera.cpp:380-415` | `buildDumlPacket` no dest-size, stack overflow risk | FIXED(this pass) | No backing for unchecked; added `bufCap` guard |
| V02 | 2 Camera proto | HIGH | `src/dji_camera.cpp:670,689` | Stale `0x0A/0x0D` comment vs `0x02/0x02` code | FIXED(this pass) | Comment drift, no backing; corrected |
| V03 | 2 Camera proto | HIGH | `src/dji_camera.cpp:505-535` | Session trust from unauth notify | OPEN | Protocol has no challenge beyond approve tap; bond proof is roadmap, not yet implemented |
| V04 | 2 Camera proto | MEDIUM | `src/dji_camera.cpp:555-558` | `flags==0x00 → dataValid=true` with zero parse | OPEN | Placeholder for future telemetry parse; marking valid early is unjustified, needs gate |
| V05 | 2 Camera proto | MEDIUM | `src/gopro_camera.cpp:346-364` | `frame[40]` unchecked `plen` | FIXED(this pass) | No backing; added bound check |
| V06 | 2 Camera proto | MEDIUM | `src/gopro_camera.cpp:429-453` | Tolerant `[id][u8]` walk misalign risk | REASONED | R-V06: OpenGoPro firmwares differ (HERO8-13); tolerant walk is deliberate per wild captures. Needs unit vectors, not strict parse |
| V07 | 2 Camera proto | MEDIUM | `src/gopro_camera.cpp:568-578` | `gpSendStartRecord` always `true` | FIXED(this pass) | No backing (copy-paste oversight vs DJI path); fixed to propagate `writeCommand` |
| V08 | 2 Camera proto | LOW | `src/dji_camera.cpp:618-630` | 30s auth timeout, no backoff | OPEN | 30s matches camera approve UX; backoff missing, needs fix |
| V09 | 2 Camera proto | INFO | — | Video backpressure N/A, mapped to BLE/WiFi radio share | REASONED | R-V09: no video FPS; 40% BLE duty + scan cooldown is the equivalent backpressure design |
| V10 | 2 Camera proto | INFO | `src/dji_camera.cpp:641` etc. | Error recovery good, `_hasTargetAddress` never cleared | OPEN | Retry-forever was simplicity; needs failure counter |
| P01 | 3 Perf | HIGH | `src/web_server.cpp:862-886` | `delay`+`softAP` blocking in POST path | OPEN | Simplicity; needs deferred restart in `webUpdate` |
| P02 | 3 Perf | MEDIUM | `src/main.cpp:196-238` | `connectToCamera` can block 10s in `loop()` | OPEN | Sync connect simplicity; needs async or 5s cap |
| P03 | 3 Perf | MEDIUM | `src/fc_status.cpp:178-192` | Identity phase blocks regular polls 1.2s | REASONED | R-P03: staggered 300ms identity avoids interleaved MSP confusion on Betaflight; 1.2s boot-only, acceptable. Could interleave later |
| P04 | 3 Perf | LOW | `src/recorder.cpp` | 1s retry polling + log spam | OPEN | Absolute start/stop makes retry safe; backoff missing |
| P05 | 3 Perf | INFO | — | Core affinity N/A (C3 single-core) | REASONED | R-P05: `CONFIG_FREERTOS_UNICORE`; pinning would break build. Cooperative `yield`+duty is correct |
| P06 | 3 Perf | INFO | — | DMA double-buffer N/A | REASONED | R-P06: no camera DMA; UART FIFO + 64B budget is equivalent |
| P07 | 3 Perf | LOW | `src/wifiswitch.cpp` / `src/web_server.cpp:933-941` | No sleep/clock gating beyond WiFi off | REASONED | R-P07: deep-sleep would drop BLE bond + MSP link; WiFi-off (60-100mA) is the safe flight saving. 80MHz idle is future |
| S01 | 4 Security | CRITICAL | `src/config.h:75-76` / `src/web_server.cpp:878-882` | Public default PSK + open fallback | OPEN (partial) | `aeebeb4` rejects weak POST + 8-63 enforced; first-boot random PSK still roadmap |
| S02 | 4 Security | CRITICAL | `src/web_server.cpp:905-914` | Zero auth on all routes incl. OTA | OPEN | AP-isolation assumed; full auth is roadmap, partial 413/429/404/CSP done |
| S03 | 4 Security | CRITICAL | `src/web_server.cpp:732-804` | Unauth unsigned OTA | OPEN (partial) | `aeebeb4` fixed dual-callback+abort+size cap; ECDSA + anti-rollback roadmap |
| S04 | 4 Security | HIGH | `src/dji_camera.cpp:442-453` | Fixed global pairing id/token | REASONED | R-S04: `osmosis` verified — camera stores approval UNDER this string; dynamic breaks silent-reconnect fast path. On-screen tap is real gate |
| S05 | 4 Security | HIGH | `src/camera_manager.cpp:95-96` | JustWorks, DJI plaintext | REASONED | R-S05: cameras offer no IO (NO_INPUT_OUTPUT); DJI DUML has no LE-SC upgrade path. Documented limitation, GoPro does `secureConnection()` |
| S06 | 4 Security | HIGH | `src/cam_registry.cpp:177-187` | Trust = MAC `strcasecmp` | OPEN | Simplicity; IRK/bond pinning roadmap |
| S07 | 4 Security | HIGH | `src/web_server.cpp` allowlist | GPS removed, MOTOR/ALT remain | REASONED | R-S07: 108 GPS dropped in `aeebeb4`; 104/109 kept because Dashboard MSP console offers them (see `web_assets.h:454,456`). Read-only, no position fix |
| S08 | 4 Security | MEDIUM | `src/web_server.cpp:67-97` | Hand JSON quirks | OPEN | Avoid-dependency simplicity; ArduinoJson is roadmap |
| S09 | 4 Security | MEDIUM | XSS (`web_assets.h:661`, `camera_manager.cpp:28-46`) | Contained but fragile | FIXED(cleanup) | `aeebeb4`: real `esc()` + pills + `sanitizeDeviceName`. FC-string JSON-escape still open |
| S10 | 4 Security | MEDIUM | `src/web_server.cpp:897-901` | Captive DNS `*` hijack | REASONED | R-S10: captive portal *requires* wildcard DNS; scope is AP-local, no upstream routing. Banner + no `Host` logging mitigates |
| S11 | 4 Security | LOW | `src/web_server.cpp:109-234` | Unauth info disclosure (heap/MACs/version) | OPEN | Debug convenience; minimize to version+scanning for unauth is roadmap |
| S12 | 4 Security | LOW | `src/dji_camera.cpp:486-491` | Hex-dump log leakage over USB | REASONED | R-S12: bench debug needs DUML hex; flight profile silences via `DEBUG_DISABLED`. Physical-USB = physical access |
| Q01 | 5 Quality | HIGH | Multiple | Unchecked returns (WiFi/Prefs/Update/NimBLE) | OPEN | Arduino examples omit checks; needs systematic `if(!x)` + `lastError` |
| Q02 | 5 Quality | MEDIUM | `src/web_server.cpp` | `String` in hot path | OPEN | Same as E02 |
| Q03 | 5 Quality | MEDIUM | `src/web_server.cpp:956` | 956-line god-file | OPEN | Split `web_json/ota/msp.cpp` roadmap |
| Q04 | 5 Quality | MEDIUM | Docs | Stale `0x0A`, 33KB vs 67KB, etc. | FIXED(this pass, partial) | `0x0A` fixed; README size + OTA warning still open |
| Q05 | 5 Quality | LOW | `platformio.ini` | No `test` env, no Unity/HIL | OPEN | Needs `test/test_msp_parse.cpp` |
| H01 | 6 Pitfalls | HIGH | — | Brownout, no `setBrownoutVoltage` | OPEN | Omitted; needs 2.7V + cap note |
| H02 | 6 Pitfalls | INFO | — | PSRAM N/A (C3 none) | REASONED | R-H02: `ESP.getPsramSize()==0` by silicon; heap-budget discipline is equivalent |
| H03 | 6 Pitfalls | N/A | — | XCLK N/A (no sensor) | REASONED | R-H03: no `esp_camera`; ignore |
| H04 | 6 Pitfalls | HIGH | `src/config.h:26-27,158-160` | GPIO8 strapping + 20/21 USB-JTAG conflict | OPEN | DevKitM-1 default; variant matrix missing |
| H05 | 6 Pitfalls | MEDIUM | — | No thermal check | OPEN | Needs `temperatureRead()` in heartbeat |
| H06 | 6 Pitfalls | HIGH | `src/dji_camera.cpp:293` / `src/gopro_camera.cpp:265` / `src/camera_manager.cpp:66-78` | BLE client leak, never `deleteClient()` | OPEN | Disconnect-only was simplicity; needs delete on remove + heap test |

## Recheck evidence (why written that way)

### R-E01 static JSON buffers
`web_server.cpp:110,587` `static char` avoids 6KB stack (C3 cont stack ~8KB) + avoids per-request `malloc`. Truncation now fail-closes. Kept.

### R-E03 std::string in scan callback
NimBLE `getAddress().toString()` returns temporary; file header `dji_camera.cpp:111-130` explicitly keeps `std::string` alive to avoid dangle. Small, short-lived. Alternative raw API (`getAddress().getNative()`) would diverge from NimBLE examples. Kept, documented.

### R-E10 67KB send_P
Single-file PROGMEM UI is deliberate (no SPIFFS/LittleFS partition pressure with `min_spiffs.csv`, OTA simplicity). `no-store` avoids stale UI after OTA. Concurrent `/` rare (single pilot). Kept + `Connection:close` future.

### R-E11 always-debug
`git log` shows bench-driven development (`pio device monitor -b 115200` in README). USB logs are primary field tool. Flight silence via build flag documented in `config.h:166-172`. Kept.

### R-V06 tolerant GoPro parse
`gopro_camera.cpp:392-396` comments “layouts seen in the wild differ”. HERO8-13 firmwares send different headers (`0xB3/0x93/0x53/0x13/0xD3`). Strict parse would break compat. Kept; needs vector tests.

### R-V09/P05/P06 N/A mappings
C3 unicore + no camera sensor verified via `platformio.ini:10`, no `esp_camera` in `src/`. Radio-share design (40ms/100ms duty `dji_camera.cpp:263-266`, 5s one-shot scan, server cooldown) is the real backpressure. Kept.

### R-P03 identity block
`fc_status.cpp:178-192` staggers 4 identity cmds 300ms to avoid interleaving with Betaflight MSP confusion + resync. Boot-only 1.2s. Arm path (`recorder.cpp`) tolerates delay (switch still live). Kept.

### R-P07 no deep-sleep
Deep/light sleep drops BLE bond + MSP UART + SoftAP. Product must stay connected in flight. WiFi-off saving is the safe equivalent (`wifiswitch`). Kept.

### R-S04 fixed pairing id/token
`dji_camera.cpp:427-438` cites `osmosis` hardware-verified: camera keys approval under `"001749319286102"` + displays `"osmo"`. Dynamic id breaks “already paired” fast path (`payload[1]==0x01`). Real gate is on-camera tap. Kept.

### R-S05 JustWorks
Both cameras expose no IO; `camera_manager.cpp:95` `BLE_HS_IO_NO_INPUT_OUTPUT` is only viable. GoPro upgrades via `secureConnection()` (`gopro_camera.cpp:318`); DJI DUML has no LE-SC path in public captures. Documented limitation. Kept.

### R-S07 MOTOR/ALT kept
`web_assets.h:454,456` MSP console offers 104/109; 108 GPS was removed in `aeebeb4` (privacy). 104/109 are read-only, no position. Kept.

### R-S10 wildcard DNS
Captive portal cannot work without `*` (`web_server.cpp:897-901`). No upstream routing (AP-local only), OS probes answered minimally. Kept + banner recommendation.

### R-S12 USB logs
DUML hex (`dji_camera.cpp:488-491`) is how `lib-osmo-ble`/`osmosis` reverse-engineering works. Physical USB implies physical drone access. Flight builds silence. Kept.

### R-H02/H03 N/A
C3 silicon has no PSRAM; no sensor, no XCLK. Heap discipline + radio-share are equivalents. Kept.

## Fixes applied in this pass (no backing found)

| ID | Fix | Files |
|----|-----|-------|
| E13 | Zero-init `MspMessage msg{}` in `mspReadIncoming` + `handleMspPost` | `src/main.cpp`, `src/web_server.cpp` |
| V01 | `buildDumlPacket` bufCap guard, return 0 on overflow; updated all callers | `src/dji_camera.cpp` |
| V02 | Correct `0x0A/0x0D` comments to `0x02/0x02` | `src/dji_camera.cpp` |
| V05 | Bound `plen+2<=sizeof(frame)` in `sendShutter` | `src/gopro_camera.cpp` |
| V07 | Propagate `writeCommand` result from `gpSendStartRecord/Stop` | `src/gopro_camera.cpp` |
| Q04-partial | Doc fixes (stale cmdSet) | `src/dji_camera.cpp` |
| E08 | Drop blocking `Serial.flush()` after MSP TX (async FIFO) | `src/msp_protocol.cpp` |
| H06-partial | `deleteClient()` on backend switch/remove path documentation + safe delete helper (no leak on switch) | `src/camera_manager.cpp` |

## Remaining OPEN (roadmap, needs owner)
E02, E04, E06-micro, E07, E08-done, E09, E12, V03, V04, V08, V10, P01, P02, P04, S01-random-PSK, S02-auth/CSRF, S03-signing, S06-IRK, S08-ArduinoJson, S09-FC-escape, S11-minimize, Q01-returns, Q03-split, Q04-README, Q05-tests, H01-brownout, H04-GPIO, H05-thermal, H06-full.

## Verification checklist (for each FIXED)
- `pio run` clean (`-Wall -Wextra`, no `handleOtaPost` refs, no `0x0A` refs).
- Host Unity vectors: `buildDumlPacket` overflow returns 0; `sendShutter` 40B+ rejected; `gpSend` false when GATT down.
- Bench: 50x connect/disconnect heap stable; MSP console + RC switch concurrent, no missed edge; OTA corrupt aborts to old FW.
- Radio: dashboard poll + BLE keepalive gap <2x interval; scan cooldown 429 verified with scripted `scan:true` flood.
