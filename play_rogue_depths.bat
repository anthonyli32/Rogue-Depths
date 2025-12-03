@echo off
REM Rogue Depths - Windows Launcher
REM This script launches the game from the release package

cd /d "%~dp0"

REM Check for Windows executable first
if exist "rogue_depths.exe" (
    echo Starting Rogue Depths...
    "rogue_depths.exe"
    goto :end
)

REM Check for Linux binary (WSL) - fallback
if exist "rogue_depths" (
    echo Starting Rogue Depths via WSL...
    wsl bash -c "cd '%~dp0' && ./rogue_depths"
    if errorlevel 1 (
        echo.
        echo Error: WSL not found or binary failed to run.
        echo Please install WSL or use the Windows executable.
        pause
    )
    goto :end
)

REM Not found
echo Error: rogue_depths executable not found!
echo.
echo Please ensure you have:
echo - rogue_depths.exe (Windows) OR
echo - rogue_depths (Linux/WSL)
echo.
echo If you're missing DLLs, run: fix_missing_dlls.bat
pause

:end

