@echo off
setlocal
powershell -ExecutionPolicy Bypass -File "%~dp0build_windows_x86.ps1" -Config Release
if errorlevel 1 exit /b %errorlevel%
echo.
echo Done.
