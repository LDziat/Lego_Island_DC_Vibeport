target remote 127.0.0.1:1235
set logging file build/serial_out.txt
set logging enabled on
break *0x8c012f7c
break *0x8c11dde8
break *0x8c012fac
continue
echo \n--- FIRST EXCEPTION DETECTED ---\n
bt
info registers
set logging enabled off
quit
