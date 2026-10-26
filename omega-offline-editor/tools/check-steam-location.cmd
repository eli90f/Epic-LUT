@echo off
setlocal
set TEMP=D:\GPT\HellForge-BuildCache\Temp
set TMP=%TEMP%
call "D:\GPT\Toolchains\VS2022\VC\Auxiliary\Build\vcvars64.bat" >nul
cd /d "%~dp0.."
cl /nologo /std:c++17 /EHsc /O2 /MT /utf-8 /W3 tests\steam_location_tests.cpp native\steam_location.cpp /Fo:build\release\ /Fe:build\release\steam-location-tests.exe
if errorlevel 1 exit /b 1
build\release\steam-location-tests.exe
exit /b %errorlevel%
