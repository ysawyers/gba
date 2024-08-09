#include <SDL.h>

#include <string>

class Window {
    public:
        Window();

        ~Window() {
            SDL_DestroyWindow(m_window);
            SDL_Quit();
        }

        void emulate(const std::string&& rom_filepath);

    private:
        void render_frame(std::uint16_t* frame_buffer);

        SDL_Window* m_window;
        SDL_Renderer* m_renderer;
};
