@echo off
setlocal
set TEMP=D:\GPT\HellForge-BuildCache\Temp
set TMP=%TEMP%
call "D:\GPT\Toolchains\VS2022\VC\Auxiliary\Build\vcvars64.bat" >nul
cd /d "%~dp0.."
cl /nologo /std:c++17 /EHsc /O2 /openmp /MT /utf-8 /W3 /c native\main.cpp /Fo:build\dev-preview\main-display-check.obj
exit /b %errorlevel%
