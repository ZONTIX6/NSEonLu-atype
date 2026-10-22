param([Parameter(Mandatory)][string]$BuildDir)
$ErrorActionPreference='Stop'
$workspace=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$target=[IO.Path]::GetFullPath((Join-Path $workspace $BuildDir))
$buildRoot=[IO.Path]::GetFullPath((Join-Path $workspace 'build'))+[IO.Path]::DirectorySeparatorChar
if(-not $target.StartsWith($buildRoot,[StringComparison]::OrdinalIgnoreCase)) {
    throw "Clean target must be a profile directory under $buildRoot"
}
if(Test-Path -LiteralPath $target) {Remove-Item -LiteralPath $target -Recurse -Force}
