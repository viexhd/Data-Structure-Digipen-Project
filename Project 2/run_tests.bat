@echo off
REM run_tests.bat — runs the test suite via WSL
REM Double-click this file from Windows Explorer, or run from cmd/PowerShell

cd /d "%~dp0"
wsl dos2unix run_tests.sh
wsl make clean
wsl bash ./run_tests.sh
pause
