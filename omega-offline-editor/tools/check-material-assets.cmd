@echo off
setlocal
set TEMP=D:\GPT\HellForge-BuildCache\Temp
set TMP=%TEMP%
call "D:\GPT\Toolchains\VS2022\VC\Auxiliary\Build\vcvars64.bat" >nul
cd /d "%~dp0.."
cl /nologo /std:c++17 /EHsc /O2 /openmp /MT /utf-8 /W3 tests\item_preview_tests.cpp native\lut_io.cpp native\game_assets.cpp native\asset_labels.cpp native\item_mesh.cpp native\item_preview.cpp native\preview_cache.cpp native\native_texture.cpp native\texture_cache.cpp native\gpu_material_preview.cpp /Fe:"build\dev-preview\material-assets-tests.exe" /Fo:build\dev-preview\ /link user32.lib gdi32.lib
exit /b %errorlevel%
