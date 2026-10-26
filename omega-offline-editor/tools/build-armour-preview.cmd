@echo off
setlocal
set TEMP=D:\GPT\HellForge-BuildCache\Temp
set TMP=%TEMP%
if not exist "%TEMP%" mkdir "%TEMP%"
call "D:\GPT\Toolchains\VS2022\VC\Auxiliary\Build\vcvars64.bat" >nul
cd /d "%~dp0..\native"
rc /nologo /fo ..\build\armor-preview\omega.res omega.rc
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /O2 /MT /utf-8 /W3 main.cpp steam_location.cpp lut_io.cpp game_assets.cpp asset_labels.cpp material_preview.cpp item_mesh.cpp item_preview.cpp preview_cache.cpp native_texture.cpp texture_cache.cpp gpu_material_preview.cpp patch_install.cpp ..\build\armor-preview\omega.res /Fe:"..\dist\OMEGA Epic LUT Editor Armour Test.exe" /Fo:..\build\armor-preview\ /link user32.lib gdi32.lib comctl32.lib comdlg32.lib dwmapi.lib shell32.lib /SUBSYSTEM:WINDOWS
if errorlevel 1 exit /b 1
dumpbin /nologo /dependents "..\dist\OMEGA Epic LUT Editor Armour Test.exe" > ..\build\armor-preview\dependencies.txt
exit /b %errorlevel%
