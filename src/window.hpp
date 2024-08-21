#ifndef WINDOW_HPP
#define WINDOW_HPP

#include <string>

#include <SDL.h>

class Window {
    public:
        void open(const std::string&& rom_filepath);
    
    private:
        void sdl_initialize(SDL_Window** window, SDL_Renderer** renderer);
};

#endif