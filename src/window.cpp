#include "window.hpp"

#include "cpu.hpp"

#include "libs/imgui/imgui.h"
#include "libs/imgui/imgui_impl_sdl2.h"
#include "libs/imgui/imgui_impl_sdlrenderer2.h"

#define RGB_VALUE(n) (((n) << 3) | ((n) >> 2))

// constexpr std::uint32_t screen_height = 160;
// constexpr std::uint32_t screen_width = 240;
// constexpr std::uint32_t screen_pixels = screen_height * screen_width;
// constexpr std::uint32_t screen_factor = 3;

void Window::sdl_initialize(SDL_Window** window, SDL_Renderer** renderer) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        throw std::runtime_error("Failed to initialize SDL\n");
    }

    SDL_Window* sdl_window = SDL_CreateWindow(
        "GBArcade",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        1000,
        650,
        SDL_WINDOW_SHOWN
    );

    if (sdl_window == nullptr) {
        SDL_Quit();
        throw std::runtime_error("Failed to initialize SDL\n");
    }

    SDL_Renderer* sdl_renderer = SDL_CreateRenderer(sdl_window, -1, 0);
    if (sdl_renderer == nullptr) {
        SDL_DestroyWindow(sdl_window);
        SDL_Quit();
        throw std::runtime_error("Failed to initialize SDL\n");
    }

    *window = sdl_window;
    *renderer = sdl_renderer;
}

void Window::open(const std::string&& rom_filepath) {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    sdl_initialize(&window, &renderer);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    bool running = true;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = false;
            }
            ImGui_ImplSDL2_ProcessEvent(&e);
        }
 
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ImGui::ShowDemoWindow(&running);

        ImGui::Render();
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
    }

    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

// void Window::render_frame(std::uint16_t* frame_buffer) {
    
// }

// void Window::emulate(const std::string&& rom_filepath) {
//     CPU cpu(std::move(rom_filepath));
    
// }

/*

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
            }
            case SDL_QUIT:
                event_loop = false;
                break;
            }
        
*/

// RGB_VALUE(frame_buffer[i] & 0x1F), RGB_VALUE((frame_buffer[i] >> 5) & 0x1F), RGB_VALUE(((frame_buffer[i] >> 10) & 0x1F))