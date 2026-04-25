@echo off
setlocal EnableExtensions
pushd "%~dp0"

set "BUILD_DIR=%CD%\build"
set "APP_EXE=%BUILD_DIR%\nimonspoli_qt_ui.exe"
set "WINDEPLOYQT="

if defined QTDIR if exist "%QTDIR%\bin\windeployqt.exe" set "WINDEPLOYQT=%QTDIR%\bin\windeployqt.exe"
if not defined WINDEPLOYQT (
    for /f "delims=" %%I in ('dir /b /s "C:\Qt\windeployqt.exe" 2^>nul') do (
        set "WINDEPLOYQT=%%I"
        goto :found_deploy
    )
)

:found_deploy
echo [Nimonspoli UI] Configuring project...
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 goto :fail

echo [Nimonspoli UI] Building project...
cmake --build build --parallel
if errorlevel 1 goto :fail

if exist "%APP_EXE%" if defined WINDEPLOYQT (
    echo [Nimonspoli UI] Deploying Qt runtime...
    "%WINDEPLOYQT%" --no-translations --no-system-d3d-compiler "%APP_EXE%"
)

if not exist "%APP_EXE%" goto :fail

set "QT_QPA_PLATFORM_PLUGIN_PATH=%BUILD_DIR%\platforms"
set "QT_PLUGIN_PATH=%BUILD_DIR%"

echo [Nimonspoli UI] Launching app...
pushd "%BUILD_DIR%"
".\nimonspoli_qt_ui.exe"
set "APP_EXIT=%ERRORLEVEL%"
popd
popd
exit /b %APP_EXIT%

:fail
echo [Nimonspoli UI] Build or launch preparation failed.
popd
pause
exit /b 1
