param(
    [string]$Python,
    [switch]$Check
)
$ErrorActionPreference = 'Stop'

# Keep this launcher ASCII so Windows PowerShell 5.1 can read it without a BOM.
$candidates = [Collections.Generic.List[object]]::new()
if ($Python) {
    $candidates.Add(@{Program=$Python; Prefix=@()})
} else {
    foreach ($name in @('python3', 'python', 'py')) {
        foreach ($command in @(Get-Command $name -CommandType Application -ErrorAction SilentlyContinue)) {
            if ($command.Source -notlike '*\Microsoft\WindowsApps\*') {
                $prefix = @()
                if ($name -eq 'py') { $prefix = @('-3') }
                $candidates.Add(@{Program=$command.Source; Prefix=$prefix})
            }
        }
    }
    $patterns = @(
        "$env:LOCALAPPDATA\Programs\Python\Python*\python.exe",
        "$env:APPDATA\uv\python\*\python.exe",
        "$env:USERPROFILE\Documents\Codex\tools\python\*\python.exe",
        "$env:USERPROFILE\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe"
    )
    foreach ($pattern in $patterns) {
        foreach ($item in @(Get-Item -Path $pattern -ErrorAction SilentlyContinue | Sort-Object FullName -Descending)) {
            $candidates.Add(@{Program=$item.FullName; Prefix=@()})
        }
    }
}

$selected = $null
foreach ($candidate in $candidates) {
    try {
        $prefix = $candidate.Prefix
        & $candidate.Program @prefix -c 'import sys; sys.exit(0 if sys.version_info >= (3, 10) else 1)' 2>$null
        if ($LASTEXITCODE -eq 0) { $selected = $candidate; break }
    } catch {
        # An invalid candidate must not prevent checking another local interpreter.
    }
}
if ($null -eq $selected) {
    throw 'Python 3.10+ was not found. Pass -Python with the full path to python.exe.'
}

$prefix = $selected.Prefix
foreach ($name in @('generate-structure-index.py', 'generate-data-instance-index.py')) {
    $scriptPath = Join-Path $PSScriptRoot $name
    $arguments = @($scriptPath)
    if ($Check) { $arguments += '--check' }
    & $selected.Program @prefix @arguments
    if ($LASTEXITCODE -ne 0) { throw "$name failed (exit $LASTEXITCODE)." }
}
