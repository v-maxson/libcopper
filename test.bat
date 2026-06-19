@echo off
setlocal

cmake --preset debug
if errorlevel 1 exit /b 1

cmake --build --preset debug
if errorlevel 1 exit /b 1

ctest --preset debug
if errorlevel 1 exit /b 1
