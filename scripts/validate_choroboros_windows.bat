@echo off
REM ============================================================
REM Choroboros Windows Validation Script
REM Run this from Command Prompt AFTER building the plugin
REM ============================================================

echo =========================================
echo   CHOROBOROS WINDOWS VALIDATION
echo   %date% %time%
echo =========================================
echo.

REM --- Find the VST3 ---
set "VST3_SYSTEM=%CommonProgramFiles%\VST3\Choroboros Beta.vst3"
set "VST3_FOUND="

if exist "%VST3_SYSTEM%" (
    set "VST3_FOUND=%VST3_SYSTEM%"
    echo   VST3 found: %VST3_SYSTEM%
) else (
    echo   VST3 not found at: %VST3_SYSTEM%
    echo   Searching build folder...

    for /r %%f in (*.vst3) do (
        if not defined VST3_FOUND (
            set "VST3_FOUND=%%f"
            echo   VST3 found: %%f
        )
    )
)

if not defined VST3_FOUND (
    echo   ERROR: No VST3 build found. Build the plugin first.
    pause
    exit /b 1
)

echo.
echo =========================================
echo   pluginval (VST3 Validation)
echo =========================================
echo.

REM --- Find pluginval ---
set "PLUGINVAL="

where pluginval >nul 2>nul
if %errorlevel% equ 0 (
    set "PLUGINVAL=pluginval"
) else if exist "C:\Program Files\pluginval\pluginval.exe" (
    set "PLUGINVAL=C:\Program Files\pluginval\pluginval.exe"
) else if exist "%USERPROFILE%\Downloads\pluginval.exe" (
    set "PLUGINVAL=%USERPROFILE%\Downloads\pluginval.exe"
)

if not defined PLUGINVAL (
    echo pluginval not found.
    echo.
    echo Download it from:
    echo   https://github.com/Tracktion/pluginval/releases
    echo.
    echo Download pluginval_Windows.zip, extract pluginval.exe
    echo Place it in C:\Program Files\pluginval\ or your Downloads folder
    echo Then run this script again.
    echo.
    pause
    exit /b 1
)

echo Using pluginval at: %PLUGINVAL%
echo.

REM --- Level 5 validation ---
set "LOG_FILE=pluginval_results_%date:~-4%%date:~4,2%%date:~7,2%.txt"

echo --- VST3 Validation: Strictness Level 5 (release minimum) ---
echo Running: %PLUGINVAL% --validate "%VST3_FOUND%" --strictness-level 5
echo This takes 2 to 10 minutes. Do not close this window.
echo Logging to: %LOG_FILE%
echo.

"%PLUGINVAL%" --validate "%VST3_FOUND%" --strictness-level 5 --timeout-ms 120000 --verbose > "%LOG_FILE%" 2>&1
set PV_EXIT=%errorlevel%

if %PV_EXIT% equ 0 (
    echo.
    echo *** pluginval Level 5: ALL TESTS PASSED ***
    echo.

    REM Try level 10
    set "LOG_FILE10=pluginval_level10_%date:~-4%%date:~4,2%%date:~7,2%.txt"
    echo --- VST3 Validation: Strictness Level 10 (comprehensive) ---
    "%PLUGINVAL%" --validate "%VST3_FOUND%" --strictness-level 10 --timeout-ms 300000 --verbose > "%LOG_FILE10%" 2>&1
    set PV10_EXIT=%errorlevel%

    if %PV10_EXIT% equ 0 (
        echo *** pluginval Level 10: ALL TESTS PASSED ***
    ) else (
        echo *** pluginval Level 10: SOME TESTS FAILED ***
        echo Check %LOG_FILE10% for details.
        echo (Level 5 passed, so this is polish, not blocking)
    )
) else (
    echo.
    echo *** pluginval Level 5: TESTS FAILED (exit code: %PV_EXIT%) ***
    echo.
    echo Check %LOG_FILE% for details.
    echo.
    echo Common exit codes:
    echo   1 = One or more tests failed
    echo   2 = Plugin not found or could not load
    echo   3 = Timeout exceeded
)

echo.
echo =========================================
echo   SUMMARY
echo =========================================
echo.
echo Log files generated:
dir /b *pluginval*.txt 2>nul
echo.
echo NEXT STEPS:
echo   1. If everything passed: you are in great shape
echo   2. If tests failed: copy the log file contents for diagnosis
echo   3. Save these logs. They are your validation evidence.
echo.
echo NOTE: Windows has no equivalent to auval.
echo VST3 validation via pluginval is the primary tool here.
echo.
pause
