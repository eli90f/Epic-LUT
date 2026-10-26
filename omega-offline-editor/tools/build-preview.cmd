@echo off
setlocal
set TEMP=D:\GPT\HellForge-BuildCache\Temp
set TMP=%TEMP%
call "D:\GPT\Toolchains\VS2022\VC\Auxiliary\Build\vcvars64.bat" >nul
cd /d "%~dp0.."
cl /nologo /std:c++17 /EHsc /O2 /openmp /MT /utf-8 /W3 tests\item_preview_tests.cpp native\lut_io.cpp native\game_assets.cpp native\asset_labels.cpp native\item_mesh.cpp native\item_preview.cpp native\preview_cache.cpp native\native_texture.cpp native\texture_cache.cpp native\gpu_material_preview.cpp /Fe:"build\dev-preview\item-preview-tests.exe" /Fo:build\dev-preview\ /link user32.lib gdi32.lib
if errorlevel 1 exit /b 1
cd native
rc /nologo /fo ..\build\dev-preview\omega.res omega.rc
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /O2 /openmp /MT /utf-8 /W3 main.cpp lut_io.cpp game_assets.cpp asset_labels.cpp material_preview.cpp item_mesh.cpp item_preview.cpp preview_cache.cpp native_texture.cpp texture_cache.cpp gpu_material_preview.cpp patch_install.cpp ..\build\dev-preview\omega.res /Fe:"..\build\dev-preview\OMEGA LUT Textured Preview Test 3.exe" /Fo:..\build\dev-preview\ /link user32.lib gdi32.lib comctl32.lib comdlg32.lib dwmapi.lib shell32.lib /SUBSYSTEM:WINDOWS
exit /b %errorlevel%
