#ifdef __DREAMCAST__
#include <kos.h>
KOS_INIT_FLAGS(INIT_DEFAULT & ~INIT_NET);
#endif
