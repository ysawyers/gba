#ifndef WINDOW_HPP
#define WINDOW_HPP

#include <memory>
#include <string>

#include "cpu.hpp"

#include <SDL.h>

class Window {
    struct MenuBarButtons {
        bool m_toggle_file_explorer = false;
        bool m_toggle_debug_panel = false;
    };

    public:
        Window() : m_menu_bar_height(0), m_pixel_height(3.5), m_cpu(nullptr) {};

        void open();

        void initialize_gba(const std::string&& rom_filepath);

    private:
        void sdl_initialize(SDL_Window** window, SDL_Renderer** renderer);

        //! initializes menu bar height member
        void render_backdrop_window();

        void render_game_window();

        float m_menu_bar_height;
        float m_pixel_height;

        MenuBarButtons m_buttons;
        std::shared_ptr<CPU> m_cpu;
};

#endif