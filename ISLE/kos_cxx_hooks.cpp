#include <kos.h>

namespace __gnu_cxx {
    void __throw_concurrence_lock_error() {
        dbglog(DBG_ERROR, "\n[CXX HOOK] __throw_concurrence_lock_error called!\n");
        uint32_t* sp;
        __asm__ __volatile__ ("mov r15, %0" : "=r" (sp));
        dbglog(DBG_ERROR, "Stack Pointer: %p\n", sp);
        
        dbglog(DBG_ERROR, "--- STACK DUMP ---\n");
        for (int i = 0; i < 64; i++) {
            uint32_t val = sp[i];
            if (val > 0x8c010000 && val < 0x8cffffff) {
                dbglog(DBG_ERROR, "SP+%02x: %08lx  <-- POSSIBLE RETURN ADDR\n", i*4, val);
            } else {
                dbglog(DBG_ERROR, "SP+%02x: %08lx\n", i*4, val);
            }
        }
        dbglog(DBG_ERROR, "------------------\n");
        arch_panic("[CXX HOOK] Caught __throw_concurrence_lock_error!");
    }
}
