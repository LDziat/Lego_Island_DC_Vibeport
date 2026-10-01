$ErrorActionPreference = "Stop"
$env:DREAMSDK_HOME = "C:\DreamSDK"
$buildDir = "C:/Users/ldzia/Desktop/AI Code Work/Lego Island/Dreamcast/isle/build"

Write-Host "Starting Flycast in GDB debug mode with ELF..."
Start-Process -FilePath "C:/Users/ldzia/Desktop/Dreamcast/flycast.exe" -ArgumentList "$buildDir/isle.elf"

Write-Host "Flycast started. Waiting 3 seconds..."
Start-Sleep -Seconds 3

Write-Host "Connecting with GDB..."
C:\DreamSDK\usr\bin\bash.exe -l -c "cd '/c/Users/ldzia/Desktop/AI Code Work/Lego Island/Dreamcast/isle/build' && sh-elf-gdb -ex 'set pagination off' -ex 'set logging file gdb_elf_output.txt' -ex 'set logging enabled on' -ex 'target remote localhost:1234' -ex 'c' -ex 'bt' -ex 'info registers' -ex 'x/16i $pc' -ex 'set logging enabled off' -ex 'quit' isle.elf"

Write-Host "Debugging complete. Output saved to build/gdb_elf_output.txt"
