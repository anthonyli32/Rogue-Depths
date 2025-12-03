@echo off
REM Rogue Depths - Development Launcher
REM This script runs the game from the development directory

cd /d "%~dp0"

REM Check for Windows executable in build/bin
if exist "build\bin\rogue_depths.exe" (
    echo Starting Rogue Depths...
    cd build\bin
    "rogue_depths.exe"
    goto :end
)

REM Check for Linux binary in build/bin (WSL)
if exist "build\bin\rogue_depths" (
    echo Starting Rogue Depths via WSL...
    wsl bash -c "cd /mnt/c/Users/Anthony/Desktop/Rogue-Depths/build/bin && ./rogue_depths"
    if errorlevel 1 (
        echo.
        echo Error: WSL not found or binary failed to run.
        pause
    )
    goto :end
)

REM Not found
echo Error: rogue_depths executable not found!
echo.
echo Please build the game first:
echo   Windows: make -f Makefile.windows
echo   Linux:   make
pause

:end

