@echo off
setlocal
set vcpkg-export=%~dp0vcpkg-export-20200207-182453\
set PATH=%PATH%;%~dp0;%vcpkg-export%installed\x64-windows\debug\bin\
echo %vcpkg-export%installed\x64-windows\debug\bin\
echo %path%

IF NOT EXIST "%~dp0%jade.exe" make all

IF EXIST "%~dp0%jade.exe" jade.exe
endlocal
