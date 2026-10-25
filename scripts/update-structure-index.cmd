@echo off
setlocal
"%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" -NoProfile -ExecutionPolicy Bypass -File "%~dp0update-structure-index.ps1" %*
set "result=%errorlevel%"
if not "%result%"=="0" echo Structure index update failed. See the error above.
if /i not "%STRUCTURE_INDEX_NO_PAUSE%"=="1" pause
exit /b %result%
