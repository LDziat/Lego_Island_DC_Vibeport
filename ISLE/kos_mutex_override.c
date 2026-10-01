// libstdc++ contains unresolved references to `mutex_lock`.
// Because `mutex_lock` is defined as `static inline` in KOS's <kos/mutex.h>, KOS's libkallisti.a
// does not provide a strong symbol for it. As a result, the linker falls back to the weak stub
// provided by libgcc.a (from fake-kos.c), which simply returns -1.
// We provide a strong `mutex_lock` symbol here to prevent libstdc++ from failing to acquire
// mutexes.

struct mutex_t;
extern int mutex_lock_timed(struct mutex_t *m, unsigned int timeout);

int mutex_lock(struct mutex_t *m) {
    return mutex_lock_timed(m, 0);
}
