@echo off
echo Building the project...
cmake -B build -G "MinGW Makefiles"
cmake --build build
if %ERRORLEVEL% equ 0 (
    echo Build successful. Running...
    cd build
    start "" "untitled.exe"
) else (
    echo Build failed!
    pause
)
