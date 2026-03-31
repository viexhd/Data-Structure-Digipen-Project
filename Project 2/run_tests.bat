@echo off
REM run_tests.bat — runs the test suite via WSL
REM Double-click this file from Windows Explorer, or run from cmd/PowerShell

cd /d "%~dp0"
rem Convert script line endings inside WSL; use dos2unix if available, otherwise sed
wsl bash -lc "if command -v dos2unix >/dev/null 2>&1; then dos2unix run_tests.sh; else sed -i 's/\r$//' run_tests.sh; fi"
wsl make clean
wsl bash ./run_tests.sh
pause
