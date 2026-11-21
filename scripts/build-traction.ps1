param(
    [ValidateRange(0,3)][int]$Mode=1,
    [switch]$Calibrated,
    [ValidateSet('Show','Build')][string]$Action='Build',
    [ValidateRange(1,64)][int]$Jobs=8
)
$ErrorActionPreference='Stop'
if($Mode -ge 2 -and -not $Calibrated) {
    throw 'Modes 2/3 require -Calibrated after verifying wheel/gyro axes and shadow thresholds. Use -Mode 1 first.'
}
$tractionRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$tractionBuild="build/traction-mode$Mode"
$tractionDirectory=Join-Path $tractionRoot $tractionBuild
[void][IO.Directory]::CreateDirectory($tractionDirectory)
$source=Get-Content -LiteralPath (Join-Path $tractionRoot 'config/00_flash/board_flash_config.h') -Raw -Encoding UTF8
# Reuse the current board-1 power, spin and communications choices. Generate a
# separate flat preset; never edit the user's normal two-board flash preset.
$overrides=@{
    BOARD_FLASH_TARGET='1'
    BOARD_FLASH_1_NAME='"Traction trial main board"'
    BOARD_FLASH_1_BUILD_DIR=('"'+$tractionBuild+'"')
    BOARD_FLASH_1_ONE_BOARD='1'
    BOARD_FLASH_1_CHASSIS_CONTROL_MODE='0'
    BOARD_FLASH_1_CHASSIS_FORCE_CALIBRATION_ENABLE='0'
    BOARD_FLASH_1_IMU_CAPTURE_ONLY='0'
    BOARD_FLASH_1_IMU_CAPTURE_CAN_TX_ENABLE='0'
    BOARD_FLASH_1_CHASSIS_IMU_CAN_RX_ENABLE='1'
    BOARD_FLASH_1_CHASSIS_VELOCITY_FUSION_ENABLE='1'
    BOARD_FLASH_1_CHASSIS_VELOCITY_FUSION_CALIBRATED='0'
    BOARD_FLASH_1_CHASSIS_REMOTE_IMU_FUSION_ENABLE='1'
    BOARD_FLASH_1_CHASSIS_TRACTION_MODE=[string]$Mode
    BOARD_FLASH_1_CHASSIS_TRACTION_CALIBRATED=([string][int]$Calibrated.IsPresent)
    BOARD_FLASH_1_VOFA_TELEMETRY_ENABLE='1'
    BOARD_FLASH_1_VOFA_TELEMETRY_MODE='2'
    BOARD_FLASH_1_VOFA_OBSERVATION_PROFILE='3'
}
foreach($key in ($overrides.Keys | Sort-Object)) {
    $pattern='(?m)^[ \t]*#define[ \t]+'+[regex]::Escape($key)+'\b[^\r\n]*'
    $definition='#define '+$key+' '+$overrides[$key]
    if([regex]::IsMatch($source,$pattern)) { $source=[regex]::Replace($source,$pattern,$definition) }
    else { $source+="`r`n$definition`r`n" }
}
$preset=Join-Path $tractionDirectory 'traction-build-config.h'
[IO.File]::WriteAllText($preset,$source,[Text.UTF8Encoding]::new($false))
& (Join-Path $PSScriptRoot 'build-board.ps1') -Action $Action -Board 1 -ConfigFile $preset -Jobs $Jobs
if($Action -eq 'Build') {
    $elf=Join-Path $tractionDirectory 'basic_framework.elf'
    if(-not (Test-Path -LiteralPath $elf -PathType Leaf)) { throw 'Traction firmware ELF was not produced.' }
    $identity=[ordered]@{ mode=$Mode; calibrated=$Calibrated.IsPresent; preset=$preset
        elf=$elf; elf_sha256=(Get-FileHash -LiteralPath $elf -Algorithm SHA256).Hash }
    $identity | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $tractionDirectory 'traction-build.json') -Encoding UTF8
    Write-Host "Built traction mode $Mode. Firmware and identity: $tractionDirectory"
}
