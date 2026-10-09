$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path -LiteralPath "$PSScriptRoot\..\..").Path
$outputDir = Join-Path $repo 'build\capture-test'
New-Item -ItemType Directory -Force -Path $outputDir | Out-Null
$fixture = Join-Path $outputDir 'capture.bin'
$csvPath = Join-Path $outputDir 'capture.csv'
& "$PSScriptRoot\test_chassis_power_capture.exe" --dump $fixture
if ($LASTEXITCODE -ne 0) { throw 'Capture fixture failed' }
& "$repo\scripts\decode-chassis-power-capture.ps1" -InputPath $fixture -OutputPath $csvPath
$rows = @(Import-Csv -LiteralPath $csvPath)
if ($rows.Count -ne 150 -or $rows[0].tick_ms -ne '70' -or
    $rows[25].target_rpm_lf -ne '1000' -or $rows[25].budget_w -ne '20') { throw 'Decoded values incorrect' }
$bytes = [IO.File]::ReadAllBytes($fixture)
$bytes[50] = $bytes[50] -bxor 1
[IO.File]::WriteAllBytes($fixture, $bytes)
$rejected = $false
try { & "$repo\scripts\decode-chassis-power-capture.ps1" -InputPath $fixture -OutputPath $csvPath }
catch { $rejected = $true }
if (!$rejected) { throw 'Corrupt frame was accepted' }
Write-Output 'PASS: capture decoder tests'
