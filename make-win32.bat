@echo off
cd "%~dp0"

if not exist build (mkdir build)
cd build

cmake -G "Visual Studio 14" -T "v140" ../ 