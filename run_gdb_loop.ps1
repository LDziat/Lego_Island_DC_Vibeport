$gdb = "C:\DreamSDK\opt\toolchains\dc\sh-elf\bin\sh-elf-gdb.exe"
Write-Host "Waiting for Flycast to start on port 1235..."
while ($true) {
    try {
        $tcpConnection = Test-NetConnection -ComputerName 127.0.0.1 -Port 1235 -InformationLevel Quiet -WarningAction SilentlyContinue
        if ($tcpConnection) {
            Write-Host "Flycast detected! Launching GDB natively..."
            & $gdb -x run.gdb "build/isle.elf"
            break
        }
    } catch {}
    Start-Sleep -Seconds 1
}
