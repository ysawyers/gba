#include "window.hpp"

#include "libs/imgui/imgui.h"
#include "libs/imgui/imgui_impl_sdl2.h"
#include "libs/imgui/imgui_impl_sdlrenderer2.h"

#define RGB_VALUE(n) (((n) << 3) | ((n) >> 2))

const int GBA_HEIGHT = 160;
const int GBA_WIDTH = 240;

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

void Window::render_backdrop_window() {
    const ImGuiWindowFlags flags = 
        ImGuiWindowFlags_NoMove | 
        ImGuiWindowFlags_NoTitleBar | 
        ImGuiWindowFlags_NoCollapse | 
        ImGuiWindowFlags_NoBringToFrontOnFocus | 
        ImGuiWindowFlags_NoResize | 
        ImGuiWindowFlags_MenuBar;

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);

    ImGui::Begin("#backdrop", nullptr, flags);
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            ImGui::MenuItem("Upload ROM");
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Debug")) {
            ImGui::MenuItem("Open Debug Window");
            ImGui::EndMenu();
        }
        m_menu_bar_height = ImGui::GetFrameHeight();
        ImGui::EndMenuBar();
    }
    ImGui::End();
}

void Window::render_game_window() {
    const ImGuiWindowFlags flags = 
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse | 
        ImGuiWindowFlags_NoResize;

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(0, m_menu_bar_height));
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::Begin("#gamewindow", nullptr, flags);

    const ImVec2 window_size = ImGui::GetWindowSize();
    const float y_offset = (window_size.y - (GBA_HEIGHT * m_pixel_height)) / 2;
    const float x_offset = (window_size.x - (GBA_WIDTH * m_pixel_height)) / 2;

    // TODO: unsafe if opened without specifying ROM currently
    FrameBuffer frame_buffer = m_cpu->render_frame(0xFFFF);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    for (int col = 0; col < GBA_HEIGHT; col++) {
        for (int row = 0; row < GBA_WIDTH; row++) {
            auto x_pos = (row * m_pixel_height) + x_offset;
            auto y_pos = (col * m_pixel_height) + y_offset;
            draw_list->AddRectFilled(ImVec2(x_pos, y_pos), ImVec2(x_pos + m_pixel_height, y_pos + m_pixel_height), 
                ImColor(
                    RGB_VALUE(frame_buffer[col][row] & 0x1F), 
                    RGB_VALUE((frame_buffer[col][row] >> 5) & 0x1F), 
                    RGB_VALUE(((frame_buffer[col][row] >> 10) & 0x1F))
                )
            );
        }
    }

    ImGui::End();
}

void Window::initialize_gba(const std::string&& rom_filepath) {
    m_cpu = std::make_shared<CPU>(std::move(rom_filepath));
}

void Window::open() {
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

        render_backdrop_window();
        render_game_window();

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

// ImGui::ShowDemoWindow(&running);

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
