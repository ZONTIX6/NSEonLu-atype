param([string]$Compiler='gcc', [Parameter(Mandatory)][string]$Flags)
# A file keeps Make's outer PowerShell from expanding this script's variables.
$compilerFlags = $Flags.Split(' ', [System.StringSplitOptions]::RemoveEmptyEntries)
foreach ($value in '-1','2','255') {
    $output = & $Compiler @compilerFlags "-DCHASSIS_FOLLOW_MODE=$value" `
        -DEXPECT_GIMBAL_FORWARD=1 -fsyntax-only test_chassis_motion.c 2>&1
    if ($LASTEXITCODE -eq 0 -or
        ($output -join [Environment]::NewLine) -notmatch 'CHASSIS_FOLLOW_MODE must be 0 or 1') {
        $output | Write-Output
        throw "Invalid mode accepted or wrong failure: $value"
    }
}
Write-Output 'PASS: invalid follow modes rejected'
