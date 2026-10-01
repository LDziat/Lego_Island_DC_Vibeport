target remote 127.0.0.1:1235

break __libc_init_array
continue
watch $sp < 0x8c800000
continue
bt
info registers
quit
