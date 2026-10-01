set mi-async on
set pagination off

target remote localhost:1234
c &
shell sleep 15
interrupt
print g_my_crash_log
quit
