#include <SDL2/SDL.h>
#include <stdio.h>
int main() {
    SDL_Init(SDL_INIT_VIDEO);
    char* path = SDL_GetPrefPath("isledecomp", "isle");
    printf("PrefPath: %p\n", path);
    SDL_Quit();
    return 0;
}
