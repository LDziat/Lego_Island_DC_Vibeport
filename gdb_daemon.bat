@echo off
title Flycast GDB Catch Crash
echo ===================================================
echo Flycast GDB Catch Crash
echo ===================================================
:loop
timeout /t 1 /nobreak >nul
netstat -ano | findstr ":1235" | findstr "LISTENING" >nul
if %errorlevel% neq 0 goto loop
echo Attaching...
C:\DreamSDK\opt\toolchains\dc\sh-elf\bin\sh-elf-gdb.exe -batch -x catch.gdb "build/isle.elf"
echo.
goto loop
