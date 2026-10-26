@echo off
set TEMP=D:\GPT\HellForge-BuildCache\Temp
set TMP=%TEMP%
call D:\GPT\Toolchains\VS2022\VC\Auxiliary\Build\vcvars64.bat >nul
cd /d D:\GPT\OMEGA-EPIC-LUT-EDITOR
cl /nologo /std:c++17 /EHsc /O2 /MT /utf-8 /W3 /c native\main.cpp native\game_assets.cpp /Fo:build\armor-preview\
if errorlevel 1 exit /b 1
link /nologo build\armor-preview\main.obj build\armor-preview\game_assets.obj build\armor-preview\steam_location.obj build\armor-preview\lut_io.obj build\armor-preview\asset_labels.obj build\armor-preview\material_preview.obj build\armor-preview\item_mesh.obj build\armor-preview\item_preview.obj build\armor-preview\preview_cache.obj build\armor-preview\native_texture.obj build\armor-preview\texture_cache.obj build\armor-preview\gpu_material_preview.obj build\armor-preview\patch_install.obj build\armor-preview\omega.res /OUT:"dist\OMEGA Epic LUT Editor Outfit Test.exe" user32.lib gdi32.lib comctl32.lib comdlg32.lib dwmapi.lib shell32.lib /SUBSYSTEM:WINDOWS
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /O2 /MT /utf-8 /W3 tests\outfit_preview_tests.cpp build\armor-preview\game_assets.obj build\armor-preview\lut_io.obj build\armor-preview\asset_labels.obj build\armor-preview\item_mesh.obj build\armor-preview\item_preview.obj build\armor-preview\preview_cache.obj build\armor-preview\native_texture.obj build\armor-preview\texture_cache.obj build\armor-preview\gpu_material_preview.obj build\armor-preview\omega.res /Fo:build\armor-preview\outfit_preview_tests.obj /Fe:build\armor-preview\outfit-preview-tests.exe /link user32.lib gdi32.lib
if errorlevel 1 exit /b 1
build\armor-preview\outfit-preview-tests.exe
