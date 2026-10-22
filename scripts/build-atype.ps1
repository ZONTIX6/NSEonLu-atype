param(
    [ValidateSet('atype-vehicle','atype-bench','atype-direct')][string]$Profile='atype-vehicle',
    [string]$BuildDir,
    [string[]]$Override=@(),
    [int]$Jobs=8
)
$ErrorActionPreference='Stop'
Push-Location (Join-Path $PSScriptRoot '..')
try {
    if (-not $BuildDir) { $BuildDir="build/$Profile" }
    & make "-j$Jobs" "PROFILE=$Profile" "BUILD_DIR=$BuildDir" @Override
    if ($LASTEXITCODE -ne 0) { throw "A-board build failed ($LASTEXITCODE)" }
} finally { Pop-Location }
