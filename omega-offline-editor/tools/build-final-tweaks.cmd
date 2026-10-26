@echo off
setlocal
set TEMP=D:\GPT\HellForge-BuildCache\Temp
set TMP=%TEMP%
call "D:\GPT\Toolchains\VS2022\VC\Auxiliary\Build\vcvars64.bat" >nul
cd /d "%~dp0..\native"
rc /nologo /fo ..\build\dev-preview\omega.res omega.rc
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /O2 /openmp /MT /utf-8 /W3 /c main.cpp /Fo:..\build\dev-preview\
if errorlevel 1 exit /b 1
cd ..
link /nologo build\dev-preview\main.obj build\dev-preview\lut_io.obj build\dev-preview\game_assets.obj build\dev-preview\asset_labels.obj build\dev-preview\material_preview.obj build\dev-preview\item_mesh.obj build\dev-preview\item_preview.obj build\dev-preview\preview_cache.obj build\dev-preview\native_texture.obj build\dev-preview\texture_cache.obj build\dev-preview\gpu_material_preview.obj build\dev-preview\patch_install.obj build\dev-preview\omega.res /OUT:"build\dev-preview\OMEGA LUT Textured Preview Test 4.exe" user32.lib gdi32.lib comctl32.lib comdlg32.lib dwmapi.lib shell32.lib /SUBSYSTEM:WINDOWS
exit /b %errorlevel%
