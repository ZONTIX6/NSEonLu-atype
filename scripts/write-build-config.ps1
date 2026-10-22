param([Parameter(Mandatory)][string]$Path, [Parameter(Mandatory)][string]$Value)
$ErrorActionPreference = 'Stop'
if (-not (Test-Path -LiteralPath $Path) -or [IO.File]::ReadAllText($Path) -ne $Value) {
    [IO.File]::WriteAllText($Path, $Value, [Text.UTF8Encoding]::new($false))
}
