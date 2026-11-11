param([Parameter(Mandatory)][string]$Python, [switch]$FirmwareMatrix, [switch]$CMake)
$ErrorActionPreference='Stop'
Push-Location (Join-Path $PSScriptRoot '..')
try {
    $logDir=Join-Path (Get-Location) 'build/validation'
    New-Item -ItemType Directory -Force $logDir | Out-Null
    $results=[Collections.Generic.List[object]]::new()
    function Run([string]$Name,[string]$Program,[string[]]$Arguments) {
        $output=& $Program @Arguments 2>&1
        $ok=$LASTEXITCODE -eq 0
        $output | Set-Content -LiteralPath (Join-Path $logDir "$Name.log") -Encoding utf8
        $results.Add([pscustomobject]@{name=$Name;passed=$ok})
        if(-not $ok) { $output | Select-Object -Last 30 | Write-Output; throw "$Name failed" }
        Write-Host "PASS $Name"
    }
    $started=(Get-Date).ToUniversalTime().ToString('o')
    $revision=git rev-parse HEAD
    $sourceStatus=@(git status --porcelain)
    Run 'architecture' $Python @('tests/architecture/test_layers.py','--strict')
    Run 'application-headers' $Python @('tests/architecture/test_application_headers.py')
    Run 'motor-link-boundary' $Python @('tests/architecture/test_motor_link.py')
    foreach($makefile in Get-ChildItem tests -Filter Makefile -Recurse) {
        Run "host-$($makefile.Directory.Name)" 'make' @('-B','-C',$makefile.Directory.FullName,'test','CC=gcc')
    }
    foreach($makefile in Get-ChildItem portable_modules -Filter Makefile -Recurse) {
        Run "portable-$($makefile.Directory.Parent.Name)" 'make' @('-B','-C',$makefile.Directory.FullName,'test','CC=gcc')
    }
    foreach($stage in 0..3) {
        Run "dji-stage$stage" 'make' @('-B','-C','tests/dji_chassis_power_rtt','test','CC=gcc',"CHASSIS_POWER_CONTROL_STAGE=$stage")
    }
    Run 'dji-no-meter' 'make' @('-B','-C','tests/dji_chassis_power_rtt','test','CC=gcc','POWER_METER_ENABLE=0')
    Run 'board-config' $Python @('tests/atype/test_board.py')
    Run 'board-flash' $Python @('-m','unittest','discover','-s','tests/board_flash','-p','test_*.py')
    Run 'node' 'node' @('--test','tools/usb-telemetry/tests/*.test.cjs','tools/waveform-viewer/tests/core.test.cjs')
    if($FirmwareMatrix) {
        $cases=@(
            @{name='atype-vehicle';flags=@('PROFILE=atype-vehicle')},
            @{name='atype-bench';flags=@('PROFILE=atype-bench')},
            @{name='atype-direct';flags=@('PROFILE=atype-direct')},
            @{name='atype-stage0';flags=@('CHASSIS_POWER_CONTROL_STAGE=0')},
            @{name='atype-stage1';flags=@('CHASSIS_POWER_CONTROL_STAGE=1')},
            @{name='atype-stage2';flags=@('CHASSIS_POWER_CONTROL_STAGE=2')},
            @{name='atype-adaptive';flags=@('CHASSIS_SPIN_MODE=1')},
            @{name='atype-sine';flags=@('CHASSIS_SPIN_MODE=2')},
            @{name='atype-no-meter';flags=@('POWER_METER_ENABLE=0')},
            @{name='atype-usb-vision';flags=@('VISION_USE_VCP=1','USB_TELEMETRY_ENABLE=0')},
            @{name='atype-capture';flags=@('CHASSIS_POWER_CAPTURE_ENABLE=1','CHASSIS_POWER_CALIBRATION_ENABLE=1','LOG_OUTPUT=2')},
            @{name='atype-chassis';flags=@('CHASSIS_BOARD=1')},
            @{name='atype-gimbal';flags=@('GIMBAL_BOARD=1')},
            @{name='atype-dual-bench';flags=@('CHASSIS_BOARD=1','PROFILE=atype-bench')},
            @{name='atype-optional';flags=@('ATYPE_LINK_MODULES=1','BOARD_OLED_ENABLE=1','BOARD_KEY_ENABLE=1','REFEREE_UART_PORT=6','BOARD_AUX_UART7_DEVICE=1','BOARD_AUX_UART2_DEVICE=2')},
            @{name='atype-i2c-oled';flags=@('ATYPE_LINK_MODULES=1','BOARD_OLED_ENABLE=1','OLED_USE_I2C=1','REFEREE_UART_PORT=6','BOARD_AUX_UART7_DEVICE=3')}
        )
        foreach($case in $cases) {
            $dir="build/$($case.name)"
            Run "make-$($case.name)" 'make' (@('-j8',"BUILD_DIR=$dir")+$case.flags)
            $checks=@('scripts/check-atype-artifacts.py',$dir)
            if($case.flags -contains 'ATYPE_LINK_MODULES=1') {$checks+='--modules'}
            Run "layout-$($case.name)" $Python $checks
        }
    }
    if($CMake) {
        foreach($profile in 'atype-vehicle','atype-bench','atype-direct') {
            $dir="build/cmake-$profile"
            Run "cmake-config-$profile" 'cmake' @('-S','.','-B',$dir,'-G','MinGW Makefiles',"-DATYPE_PROFILE=$profile")
            Run "cmake-build-$profile" 'cmake' @('--build',$dir,'-j8')
            Run "cmake-layout-$profile" $Python @('scripts/check-atype-artifacts.py',$dir)
        }
    }
} finally {
    if($null -ne $results) {
        $results | ConvertTo-Json -Depth 3 | Set-Content (Join-Path $logDir 'results.json') -Encoding utf8
        [pscustomobject]@{revision=$revision;sourceStatus=$sourceStatus;startedUtc=$started;
            finishedUtc=(Get-Date).ToUniversalTime().ToString('o');commands=$results.Count;
            passed=@($results | Where-Object passed).Count} |
            ConvertTo-Json -Depth 3 | Set-Content (Join-Path $logDir 'provenance.json') -Encoding utf8
    }
    Pop-Location
}
