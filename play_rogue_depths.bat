@echo off
REM Rogue Depths - Windows Launcher
REM This script launches the game from the project root directory

cd /d "%~dp0"
if exist "build\bin\rogue_depths.exe" (
    echo Starting Rogue Depths...
    "build\bin\rogue_depths.exe"
) else (
    echo Error: rogue_depths.exe not found!
    echo Please build the game first using: make
    pause
)

