
#include <kos.h>
#include <sys/stat.h>
#include <stdio.h>

KOS_INIT_FLAGS(INIT_DEFAULT);

int main() {
    struct stat st;
    printf("Testing stat...\n");
    if (stat("/cd/lego/scripts/credits.si", &st) == 0) {
        printf("Found lowercase!\n");
    } else {
        printf("Lowercase failed!\n");
    }
    if (stat("/cd/LEGO/Scripts/CREDITS.SI", &st) == 0) {
        printf("Found exact case!\n");
    } else {
        printf("Exact case failed!\n");
    }
    return 0;
}
