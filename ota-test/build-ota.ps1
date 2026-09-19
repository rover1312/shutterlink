# Requires: PlatformIO (pio) on PATH. Run from repo root.
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
Set-Location $root
pio run -e esp32c3
$bin = Join-Path $root '.pio\build\esp32c3\firmware.bin'
if (!(Test-Path $bin)) { throw "Build output missing: $bin" }
Copy-Item $bin (Join-Path $PSScriptRoot 'firmware.bin') -Force
$size = (Get-Item (Join-Path $PSScriptRoot 'firmware.bin')).Length
"firmware.bin: $size bytes"
"Built from $(git rev-parse --short HEAD) on $(git branch --show-current)"
"Next: join ShutterLink AP, open http://192.168.4.1, FC/System -> Firmware Update."
