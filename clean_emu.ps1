$lines = Get-Content "C:\Users\ldzia\Desktop\Dreamcast\emu.cfg"
$newLines = @()
foreach ($line in $lines) {
    if ($line -notmatch "GDB") {
        $newLines += $line
    }
}
$newLines += "[config]"
$newLines += "Debug.GDBEnabled = yes"
$newLines += "Debug.GDBPort = 1234"
$newLines += "Debug.GDBWaitForConnection = yes"
Set-Content -Path "C:\Users\ldzia\Desktop\Dreamcast\emu.cfg" -Value $newLines
