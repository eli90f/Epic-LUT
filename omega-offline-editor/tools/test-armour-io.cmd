@echo off
set TEMP=D:\GPT\HellForge-BuildCache\Temp
set TMP=%TEMP%
call D:\GPT\Toolchains\VS2022\VC\Auxiliary\Build\vcvars64.bat >nul
cd /d D:\GPT\OMEGA-EPIC-LUT-EDITOR
cl /nologo /std:c++17 /EHsc /O2 /MT /utf-8 tests\io_tests.cpp build\armor-preview\lut_io.obj /Fo:build\armor-preview\io_tests.obj /Fe:build\armor-preview\io-tests.exe
if errorlevel 1 exit /b 1
build\armor-preview\io-tests.exe build\armor-preview\io-results
