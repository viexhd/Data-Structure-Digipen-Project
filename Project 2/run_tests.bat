@echo off
REM run_tests.bat — runs the test suite via WSL
REM Double-click this file from Windows Explorer, or run from cmd/PowerShell

set "SCRIPT_DIR=%~dp0"
for /f "delims=" %%i in ('wsl wslpath -a "%SCRIPT_DIR%"') do set "WSL_SCRIPT_DIR=%%i"
wsl bash -lc "cd \"%WSL_SCRIPT_DIR%\" && sed -i 's/\r$//' run_tests.sh && bash run_tests.sh"
pause
