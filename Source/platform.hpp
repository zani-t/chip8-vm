#ifndef PLATFORM_H
#define PLATFORM_H

#include <cstdint>
#include "SDL.h"

class Platform {
public:
    Platform();

    void Update(void const* buffer, int pitch);

    bool ProcessInput(uint8_t* keys);
    
private:
    SDL_Window* window{};
    SDL_Renderer* renderer{};
    SDL_Texture* texture{};
};

#endif