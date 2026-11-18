# Build in WSL (ESP-IDF) and flash a CoreS3 over a Windows COM port.
#   .\tools\flash.ps1                # build + flash on COM7
#   .\tools\flash.ps1 -NoBuild       # flash the last build only
#   .\tools\flash.ps1 -Port COM5
# Needs: WSL distro with ESP-IDF in ~/esp/esp-idf-v6, SDK in ~/muse-gadget-sdk,
# this repo in ~/stackchan-muse (git pull'd); PlatformIO's esptool; Python.
# Close any serial monitor on the port first.
param(
    [string]$Port = 'COM7',
    [string]$Distro = 'Ubuntu-22.04',
    [string]$WslUser = '',   # default: the WSL distro's default user
    [switch]$NoBuild
)
$ErrorActionPreference = 'Stop'

$repo = Split-Path -Parent $PSScriptRoot
$out  = Join-Path $repo 'build-out'
$esptool = Join-Path $env:USERPROFILE '.platformio\packages\tool-esptoolpy\esptool.py'
if (-not (Test-Path $esptool)) { throw "esptool not found: $esptool (install PlatformIO)" }

if (-not $NoBuild) {
    Write-Host '== build (WSL) =='
    $cmd = '. ~/esp/esp-idf-v6/export.sh >/dev/null 2>&1; cd ~/stackchan-muse && git pull -q && SDK_DIR=~/muse-gadget-sdk tools/build.sh build'
    wsl.exe -d $Distro -e bash -lc $cmd
    if ($LASTEXITCODE -ne 0) { throw "build failed (exit $LASTEXITCODE)" }
}

Write-Host '== copy binaries =='
if (-not $WslUser) { $WslUser = (wsl.exe -d $Distro -e whoami).Trim() }
$src = "\\wsl.localhost\$Distro\home\$WslUser\muse-gadget-sdk\esp32\build-muse-m5stack-cores3-stackchan"
if (-not (Test-Path "$src\muse-gadget.bin")) { throw "no build output at $src (run without -NoBuild)" }
New-Item -ItemType Directory -Force $out | Out-Null
Copy-Item "$src\bootloader\bootloader.bin", "$src\partition_table\partition-table.bin", `
          "$src\ota_data_initial.bin", "$src\muse-gadget.bin" $out -Force

Write-Host "== flash $Port =="
Push-Location $out
try {
    python $esptool --chip esp32s3 --port $Port --baud 460800 `
        --before default_reset --after hard_reset write_flash `
        --flash_mode dio --flash_size keep --flash_freq 80m `
        0x0 bootloader.bin 0x10000 partition-table.bin 0x1d000 ota_data_initial.bin 0x20000 muse-gadget.bin
    if ($LASTEXITCODE -ne 0) { throw "flash failed (exit $LASTEXITCODE). Close serial monitors; or hold RST ~3s for bootloader." }
} finally { Pop-Location }
Write-Host 'done'
