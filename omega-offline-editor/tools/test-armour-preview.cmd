@echo off
set TEMP=D:\GPT\HellForge-BuildCache\Temp
set TMP=%TEMP%
call D:\GPT\Toolchains\VS2022\VC\Auxiliary\Build\vcvars64.bat >nul
cd /d D:\GPT\OMEGA-EPIC-LUT-EDITOR
cl /nologo /std:c++17 /EHsc /O2 /MT /utf-8 /c native\game_assets.cpp /Fo:build\armor-preview\game_assets.obj
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /O2 /MT /utf-8 tests\armor_catalog_tests.cpp build\armor-preview\game_assets.obj build\armor-preview\lut_io.obj build\armor-preview\asset_labels.obj build\armor-preview\item_mesh.obj build\armor-preview\preview_cache.obj build\armor-preview\native_texture.obj build\armor-preview\texture_cache.obj build\armor-preview\omega.res /Fo:build\armor-preview\armor_catalog_tests.obj /Fe:build\armor-preview\armor-catalog-tests.exe /link user32.lib gdi32.lib
if errorlevel 1 exit /b 1
build\armor-preview\armor-catalog-tests.exe
