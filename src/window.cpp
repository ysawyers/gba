#include "window.hpp"

#include <iostream>

#include "cpu.hpp"

#define RGB_VALUE(n) (((n) << 3) | ((n) >> 2))

constexpr std::uint32_t screen_height = 160;
constexpr std::uint32_t screen_width = 240;
constexpr std::uint32_t screen_pixels = screen_height * screen_width;
constexpr std::uint32_t screen_factor = 3;

Window::Window() : m_window(nullptr), m_renderer(nullptr) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        throw std::runtime_error(SDL_GetError());
    }

    m_window = SDL_CreateWindow("GBA", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
        screen_width * screen_factor, screen_height * screen_factor, SDL_WINDOW_SHOWN);
    if(m_window == nullptr) {
        throw std::runtime_error(SDL_GetError());
    }

    m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (m_renderer == nullptr) {
        SDL_DestroyWindow(m_window);
        SDL_Quit();
        throw std::runtime_error(SDL_GetError());
    }
}

void Window::render_frame(std::uint16_t* frame_buffer) {
    SDL_RenderClear(m_renderer);

    SDL_Rect lcd[screen_pixels];
    for (int i = 0; i < screen_height; i++) {
        for (int j = 0; j < screen_width; j++) {
            lcd[(i * 240) + j].w = screen_factor;
            lcd[(i * 240) + j].h = screen_factor;
            lcd[(i * 240) + j].x = j * screen_factor;
            lcd[(i * 240) + j].y = i * screen_factor;
        }
    }

    for (int i = 0; i < screen_pixels; i++) {
        SDL_SetRenderDrawColor(m_renderer, RGB_VALUE(frame_buffer[i] & 0x1F), RGB_VALUE((frame_buffer[i] >> 5) & 0x1F), RGB_VALUE(((frame_buffer[i] >> 10) & 0x1F)), 255);
        SDL_RenderFillRect(m_renderer, &lcd[i]);
    }

    SDL_RenderPresent(m_renderer);
}

void Window::emulate(const std::string&& rom_filepath) {
    CPU cpu(std::move(rom_filepath));

    SDL_Event event;
    bool event_loop = true;
    while(event_loop)
    {
        uint16_t key_input = 0xFFFF;
        while(SDL_PollEvent(&event))
        {
            switch (event.type) {
            case SDL_KEYDOWN: {
                switch (event.key.keysym.sym) {
                case SDLK_q: // A
                    key_input = ~1 & key_input;
                    break;
                case SDLK_w: // B
                    key_input = ~(1 << 1) & key_input;
                    break;
                case SDLK_RETURN: // START
                    key_input = ~(1 << 3) & key_input;
                    break;
                case SDLK_BACKSPACE: // SELECT
                    key_input = ~(1 << 2) & key_input;
                    break;
                case SDLK_RIGHT:
                    key_input = ~(1 << 4) & key_input;
                    break;
                case SDLK_LEFT:
                    key_input = ~(1 << 5) & key_input;
                    break;
                case SDLK_UP:
                    key_input = ~(1 << 6) & key_input;
                    break;
                case SDLK_DOWN:
                    key_input = ~(1 << 7) & key_input;
                    break;
                }
                break;


                break;
            }
            case SDL_QUIT:
                event_loop = false;
                break;
            }
        }

        auto frame_buffer = reinterpret_cast<std::uint16_t*>(cpu.render_frame().data());
        render_frame(frame_buffer);
    }
}
