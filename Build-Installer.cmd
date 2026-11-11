@echo off
setlocal
cd /d "%~dp0"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Installer\Build-Installer.ps1" %*
set "RABASS_RESULT=%ERRORLEVEL%"
if not defined CI pause
exit /b %RABASS_RESULT%
