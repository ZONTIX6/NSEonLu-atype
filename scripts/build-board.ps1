param(
    [ValidateSet('Show','Build','Flash','Debug','Symbols')][string]$Action='Build',
    [ValidateRange(0,2)][int]$Board=0,
    [string]$ConfigFile=(Join-Path $PSScriptRoot '../config/00_flash/board_flash_config.h'),
    [ValidateRange(1,64)][int]$Jobs=8,
    [ValidateSet('DAP','JLink')][string]$Programmer
)
$ErrorActionPreference='Stop'
$OutputEncoding=[Text.UTF8Encoding]::new($false)
[Console]::OutputEncoding=$OutputEncoding
$boardProjectRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
# VS Code reads this ELF after preLaunchTask. Invalidate it even if config/build fails,
# so choosing "Debug Anyway" cannot load a previous board's firmware or symbols.
$debugElf=Join-Path $boardProjectRoot 'build/selected-board.elf'
if($Action -in @('Debug','Symbols')) {
    if(Test-Path -LiteralPath $debugElf) { Remove-Item -LiteralPath $debugElf -Force }
}

# Read only the documented flat constant format, never execute configuration text.
$source=Get-Content -LiteralPath $ConfigFile -Raw -Encoding UTF8
$source=[regex]::Replace($source,'/\*.*?\*/',' ',[Text.RegularExpressions.RegexOptions]::Singleline)
$source=[regex]::Replace($source,'//[^\r\n]*','')
$settings=@{}
foreach($definition in [regex]::Matches($source,'(?m)^\s*#define\s+(BOARD_FLASH_\w+)[ \t]+([^\r\n]+)')) {
    $key=$definition.Groups[1].Value
    $literal=$definition.Groups[2].Value.Trim()
    if($settings.ContainsKey($key)) { throw "Duplicate setting: $key" }
    if($literal -match '^"([^"\r\n]*)"$') { $settings[$key]=$Matches[1] }
    elseif($literal -match '^\d+$') { $settings[$key]=$literal }
    else { throw "Use a single integer or quoted string for $key" }
}
function Setting([string]$Key) {
    if(-not $settings.ContainsKey($Key)) { throw "Missing setting: $Key" }
    return $settings[$Key]
}
$selected=Setting 'BOARD_FLASH_TARGET'
if($selected -notin @('1','2')) { throw 'BOARD_FLASH_TARGET must be 1 or 2.' }
if($Board -ne 0) { $selected=[string]$Board }
$programmerId=Setting 'BOARD_FLASH_PROGRAMMER'
if($programmerId -notin @('1','2')) { throw 'BOARD_FLASH_PROGRAMMER must be 1 (DAP) or 2 (JLink).' }
if($Programmer) { $programmerId=if($Programmer -eq 'DAP') { '1' } else { '2' } }
$profile=Setting 'BOARD_FLASH_PROFILE'
if($profile -notin @('atype-vehicle','atype-bench','atype-direct')) { throw 'Invalid BOARD_FLASH_PROFILE.' }
$board1Dir=Setting 'BOARD_FLASH_1_BUILD_DIR'
$board2Dir=Setting 'BOARD_FLASH_2_BUILD_DIR'
foreach($directory in @($board1Dir,$board2Dir)) {
    if($directory -notmatch '^build/[A-Za-z0-9_-]+(?:/[A-Za-z0-9_-]+)*$') {
        throw 'Board build directories must be simple relative paths below build/.'
    }
}
if($board1Dir -ieq $board2Dir) { throw 'The two boards must use different build directories.' }
$prefix="BOARD_FLASH_${selected}_"
$buildDirectory=Setting ($prefix+'BUILD_DIR')
$name=Setting ($prefix+'NAME')
$allowed=@(Get-Content -LiteralPath (Join-Path $boardProjectRoot 'config/99_build/atype-options.txt'))
$optionValues=@{}
foreach($key in $settings.Keys) {
    if(-not $key.StartsWith($prefix)) { continue }
    $option=$key.Substring($prefix.Length)
    if($option -in @('NAME','BUILD_DIR')) { continue }
    if($option -notin $allowed) { throw "Unsupported build option: $option" }
    if($settings[$key] -notmatch '^[A-Za-z0-9_.+-]+$') { throw "Invalid option value: $key" }
    $optionValues[$option]=$settings[$key]
}
# Share the firmware default; an explicit legacy/custom board setting still overrides it.
if(-not $optionValues.ContainsKey('VOFA_OBSERVATION_PROFILE')) {
    $vofaConfig=Join-Path $boardProjectRoot 'config/_vofa/vofa_config.h'
    $vofaSource=Get-Content -LiteralPath $vofaConfig -Raw -Encoding UTF8
    $vofaSource=[regex]::Replace($vofaSource,'/\*.*?\*/',' ',[Text.RegularExpressions.RegexOptions]::Singleline)
    $vofaSource=[regex]::Replace($vofaSource,'//[^\r\n]*','')
    $vofaDefinitions=[regex]::Matches($vofaSource,'(?m)^[ \t]*#define[ \t]+VOFA_OBSERVATION_PROFILE\b([^\r\n]*)')
    if($vofaDefinitions.Count -ne 1) {
        throw 'config/_vofa/vofa_config.h must define VOFA_OBSERVATION_PROFILE exactly once.'
    }
    $vofaValue=$vofaDefinitions[0].Groups[1].Value.Trim()
    if($vofaValue -notmatch '^\d+$') {
        throw 'Use a single integer for VOFA_OBSERVATION_PROFILE in config/_vofa/vofa_config.h.'
    }
    $optionValues['VOFA_OBSERVATION_PROFILE']=$vofaValue
}
# Require explicit role switches so changing boards cannot inherit unsafe defaults.
foreach($required in @('ONE_BOARD','CHASSIS_CONTROL_MODE','CHASSIS_FORCE_CALIBRATION_ENABLE',
    'ATYPE_BENCH_MODE','IMU_CAPTURE_ONLY','IMU_CAPTURE_CAN_TX_ENABLE','CHASSIS_IMU_CAN_RX_ENABLE',
    'CHASSIS_VELOCITY_FUSION_ENABLE','CHASSIS_VELOCITY_FUSION_CALIBRATED','CHASSIS_REMOTE_IMU_FUSION_ENABLE',
    'VOFA_TELEMETRY_ENABLE','VOFA_TELEMETRY_MODE','VOFA_OBSERVATION_PROFILE','USB_TELEMETRY_ENABLE')) {
    if(-not $optionValues.ContainsKey($required)) { throw "Missing board option: $prefix$required" }
}
if($optionValues['VOFA_OBSERVATION_PROFILE'] -notin @('0','1','2','3')) {
    throw 'VOFA_OBSERVATION_PROFILE must be 0, 1, 2 or 3.'
}
$options=@($optionValues.Keys | Sort-Object | ForEach-Object { "$_=$($optionValues[$_])" })
$common=@("PROFILE=$profile","BUILD_DIR=$buildDirectory")+$options
$buildArguments=@("-j$Jobs",'all')+$common
$programmer=if($programmerId -eq '1') { 'DAP' } else { 'JLink' }
$downloadTarget=if($programmerId -eq '1') { 'download_dap' } else { 'download_jlink' }
$flashArguments=@($downloadTarget)+$common
if($Action -eq 'Show') {
    [ordered]@{board=[int]$selected;name=$name;profile=$profile;buildDirectory=$buildDirectory;
        options=$options;programmer=$programmer;buildArguments=$buildArguments;
        flashArguments=$flashArguments} | ConvertTo-Json -Depth 3
    exit 0
}

function Find-ArmGnuToolchainBin {
    $compiler=Get-Command arm-none-eabi-gcc -CommandType Application -ErrorAction SilentlyContinue | Select-Object -First 1
    if($compiler) { return Split-Path -Parent $compiler.Source }

    $roots=@()
    if($env:CUBE_BUNDLE_PATH) {
        $roots+=Join-Path $env:CUBE_BUNDLE_PATH 'gnu-tools-for-stm32'
    }
    if($env:LOCALAPPDATA) {
        $roots+=Join-Path $env:LOCALAPPDATA 'stm32cube/bundles/gnu-tools-for-stm32'
    }
    $candidates=@(foreach($root in ($roots | Select-Object -Unique)) {
        foreach($versionDirectory in (Get-ChildItem -LiteralPath $root -Directory -ErrorAction SilentlyContinue)) {
            $bin=Join-Path $versionDirectory.FullName 'bin'
            if(Test-Path -LiteralPath (Join-Path $bin 'arm-none-eabi-gcc.exe') -PathType Leaf) {
                $versionText='0.0.0'
                if($versionDirectory.Name -match '^\d+(?:\.\d+)+') { $versionText=$Matches[0] }
                [pscustomobject]@{ Version=[version]$versionText; Bin=$bin }
            }
        }
    })
    if($candidates.Count -eq 0) { return $null }
    return ($candidates | Sort-Object Version -Descending | Select-Object -First 1).Bin
}

function Find-OpenOcdExecutable {
    if($env:OPENOCD_PATH) {
        $configuredPath=[Environment]::ExpandEnvironmentVariables($env:OPENOCD_PATH)
        if(Test-Path -LiteralPath $configuredPath -PathType Container) {
            $configuredPath=Join-Path $configuredPath 'openocd.exe'
        }
        if(Test-Path -LiteralPath $configuredPath -PathType Leaf) {
            return [IO.Path]::GetFullPath($configuredPath)
        }
        throw "OPENOCD_PATH does not point to openocd.exe: $configuredPath"
    }

    $command=Get-Command openocd -CommandType Application -ErrorAction SilentlyContinue | Select-Object -First 1
    if($command) { return $command.Source }

    $settingsPath=Join-Path $boardProjectRoot '.vscode/settings.json'
    if(Test-Path -LiteralPath $settingsPath -PathType Leaf) {
        $settings=Get-Content -LiteralPath $settingsPath -Raw -Encoding UTF8 | ConvertFrom-Json
        $configuredPath=[Environment]::ExpandEnvironmentVariables($settings.'cortex-debug.openocdPath')
        if($configuredPath -and (Test-Path -LiteralPath $configuredPath -PathType Leaf)) {
            return [IO.Path]::GetFullPath($configuredPath)
        }
    }
    return $null
}

if($Action -ne 'Symbols') {
    $gccBin=Find-ArmGnuToolchainBin
    if(-not $gccBin) {
        throw 'Arm GNU Toolchain not found on PATH or in the STM32Cube bundles directory. Add arm-none-eabi-gcc to PATH or install it under %LOCALAPPDATA%\stm32cube\bundles\gnu-tools-for-stm32.'
    }
    $env:PATH="$gccBin;$env:PATH"
    Write-Host "Arm GNU Toolchain: $gccBin"
}
if($Action -eq 'Flash' -and $programmerId -eq '1') {
    $openOcdExe=Find-OpenOcdExecutable
    if(-not $openOcdExe) {
        throw 'OpenOCD not found. Set OPENOCD_PATH to openocd.exe or configure cortex-debug.openocdPath in .vscode/settings.json.'
    }
    $env:PATH="$(Split-Path -Parent $openOcdExe);$env:PATH"
    Write-Host "OpenOCD: $openOcdExe"
}

Write-Host "Board $selected : $name"
Write-Host "Output: $buildDirectory"
if($Action -eq 'Flash') { Write-Host "Flash programmer: $programmer" }
Push-Location $boardProjectRoot
try {
    # Attach uses the existing firmware's symbols without rebuilding or downloading.
    if($Action -ne 'Symbols') {
        & make @buildArguments
        if($LASTEXITCODE -ne 0) { throw "Board $selected build failed ($LASTEXITCODE); download was not started." }
    }
    if($Action -in @('Debug','Symbols')) {
        $firmware=Join-Path $boardProjectRoot "$buildDirectory/basic_framework.elf"
        if(-not (Test-Path -LiteralPath $firmware -PathType Leaf)) { throw "Built firmware is missing: $firmware" }
        Copy-Item -LiteralPath $firmware -Destination $debugElf -Force
        Write-Host "Debug ELF: build/selected-board.elf <- $buildDirectory/basic_framework.elf"
    }
    if($Action -eq 'Flash') {
        $extension=if($programmerId -eq '1') { 'bin' } else { 'hex' }
        $firmware=Join-Path $boardProjectRoot "$buildDirectory/basic_framework.$extension"
        if(-not (Test-Path -LiteralPath $firmware -PathType Leaf)) { throw "Built firmware is missing: $firmware" }
        & make @flashArguments
        if($LASTEXITCODE -ne 0) { throw "Board $selected download failed ($LASTEXITCODE)." }
    }
} finally { Pop-Location }
