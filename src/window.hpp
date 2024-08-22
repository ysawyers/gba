#ifndef WINDOW_HPP
#define WINDOW_HPP

#include <memory>
#include <string>

#include "cpu.hpp"

#include <SDL.h>

class Window {
    struct MenuBar {
        MenuBar() : m_toggle_debug_panel(false), m_toggle_demo_window(false), m_toggle_file_explorer(false) {};

        bool m_toggle_debug_panel;
        bool m_toggle_demo_window;
        bool m_toggle_file_explorer;
    };

    public:
        Window() : m_menu_bar_height(0), m_cpu(nullptr), m_inserted_rom("##NONE") {};

        void open();

        void initialize_gba(const std::string&& rom_filepath);

    private:
        void sdl_initialize(SDL_Window** window, SDL_Renderer** renderer);

        //! initializes menu bar height member
        void render_backdrop_window();

        void render_game_window();

        void render_debug_window();

        float m_menu_bar_height;

        MenuBar m_menu_bar;
        std::shared_ptr<CPU> m_cpu;
        std::string m_inserted_rom;
};

#endif