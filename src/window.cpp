#include "window.hpp"

#include <filesystem>
#include <string>

#include "debugger.hpp"

#include "libs/imgui/imgui.h"
#include "libs/imgui/imgui_impl_sdl2.h"
#include "libs/imgui/imgui_impl_sdlrenderer2.h"

#define RGB_VALUE(n) (((n) << 3) | ((n) >> 2))

const int GBA_HEIGHT = 160;
const int GBA_WIDTH = 240;

void Window::initialize_gba(const std::string&& rom_filepath) {
    m_inserted_rom = std::filesystem::path(rom_filepath).filename();
    m_cpu = std::make_shared<CPU>(rom_filepath);
    m_debugger = std::make_unique<Debugger>(m_cpu);
}

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

    ImGui::Begin("##BACKGROUND_WINDOW", nullptr, flags);
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            ImGui::MenuItem("Upload ROM");
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Debug")) {
            ImGui::MenuItem("Debug Panel", nullptr, &m_menu_bar.m_toggle_debug_panel);
            ImGui::MenuItem("ImGui Demo", nullptr, &m_menu_bar.m_toggle_demo_window);
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
    const float game_window_width = m_menu_bar.m_toggle_debug_panel ? (2.5 * GBA_WIDTH)
        : viewport->Size.x;
    ImGui::SetNextWindowPos(ImVec2(0, m_menu_bar_height));
    ImGui::SetNextWindowSize(ImVec2(game_window_width, viewport->Size.y));
    ImGui::Begin("##GAME_WINDOW", nullptr, flags);

    // TODO: unsafe if opened without specifying ROM currently

    std::uint16_t key_input = 0xFFFF;
    for (ImGuiKey key = static_cast<ImGuiKey>(0); key < ImGuiKey_NamedKey_END; key = static_cast<ImGuiKey>(key + 1)) {
        if (!ImGui::IsKeyDown(key)) continue;
        switch (key) {
        case ImGuiKey_Q: // A
            key_input = ~1 & key_input;
            break;
        case ImGuiKey_W: // B
            key_input = ~(1 << 1) & key_input;
            break;
        case ImGuiKey_Backspace: // SELECT
            key_input = ~(1 << 2) & key_input;
            break;
        case ImGuiKey_Enter: // START
            key_input = ~(1 << 3) & key_input;
            break;
        case ImGuiKey_RightArrow:
            key_input = ~(1 << 4) & key_input;
            break;
        case ImGuiKey_LeftArrow:
            key_input = ~(1 << 5) & key_input;
            break;
        case ImGuiKey_UpArrow:
            key_input = ~(1 << 6) & key_input;
            break;
        case ImGuiKey_DownArrow:
            key_input = ~(1 << 7) & key_input;
            break;
        default: break;
        }
    }

    const ImVec2 window_size = ImGui::GetWindowSize();
    const float pixel_size = m_menu_bar.m_toggle_debug_panel ? 2.5 : 3.5;
    const float y_offset = ((window_size.y - (GBA_HEIGHT * pixel_size)) / 2);
    const float x_offset = ((window_size.x - (GBA_WIDTH * pixel_size)) / 2) * !m_menu_bar.m_toggle_debug_panel;

    const auto& frame_buffer = m_breakpoint_reached ? m_cpu->view_current_frame() : m_cpu->render_frame(key_input, m_breakpoint, m_breakpoint_reached);
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    for (int col = 0; col < GBA_HEIGHT; col++) {
        for (int row = 0; row < GBA_WIDTH; row++) {
            auto x_pos = (row * pixel_size) + x_offset;
            auto y_pos = (col * pixel_size) + y_offset;
            draw_list->AddRectFilled(ImVec2(x_pos, y_pos), ImVec2(x_pos + pixel_size, y_pos + pixel_size), 
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

void Window::render_debug_window() {
    const ImGuiWindowFlags flags = 
        ImGuiWindowFlags_NoMove | 
        ImGuiWindowFlags_NoCollapse | 
        ImGuiWindowFlags_NoResize;

    const int game_window_width = (2.5 * GBA_WIDTH);
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(game_window_width, m_menu_bar_height));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x - game_window_width, viewport->Size.y));
    ImGui::Begin("Debug Panel", &m_menu_bar.m_toggle_debug_panel, flags);

    if (ImGui::BeginChild(ImGui::GetID("instr_view"), ImVec2(-1, 250), ImGuiChildFlags_Border)) {
        auto instrs = m_debugger->view_nearby_instructions();
        for (const auto& instr : instrs) {
            if (m_debugger->current_pc() == instr.addr) {
                ImGui::TextColored(ImVec4(1, 1, 0, 1), "%08X %08X %s", instr.addr, instr.opcode, instr.desc.c_str());
                ImGui::SetScrollHereY(0.5f);
            } else {
                ImGui::Text("%08X %08X %s", instr.addr, instr.opcode, instr.desc.c_str());
            }
        }
    }
    ImGui::EndChild();

    ImGui::Spacing();
    if (ImGui::Button("Set Breakpoint")) {
        ImGui::OpenPopup("Set Breakpoint");
    }
    render_breakpoint_modal();
    ImGui::SameLine();
    if (ImGui::Button("Stop")) {
        m_breakpoint_reached = true;
    }
    ImGui::SameLine();
    if (m_breakpoint_reached && ImGui::Button("Resume")) {
        m_breakpoint_reached = false;
        m_breakpoint = 0xFFFFFFFF;
    }
    if (m_breakpoint_reached && ImGui::Button("Step")) {
        m_cpu->step();
    }
    ImGui::SameLine();
    if (m_breakpoint_reached && ImGui::Button("Step 10")) {
        for (int i = 0; i < 10; i++) {
            m_cpu->step();
        }
    }
    ImGui::SameLine();
    if (m_breakpoint_reached && ImGui::Button("Step 100")) {
        for (int i = 0; i < 100; i++) {
            m_cpu->step();
        }
    }
    ImGui::SameLine();
    if (m_breakpoint_reached && ImGui::Button("Step 1000")) {
        for (int i = 0; i < 100000; i++) {
            m_cpu->step();
        }
    }
    ImGui::Spacing();

    if (ImGui::BeginTable("registers", 2)) {
        auto& regs = m_debugger->view_registers();
        for (int row = 0; row < 8; row++) {
            ImGui::TableNextRow();
            for (int col = 0; col < 2; col++) {
                auto reg = (8 * col) + row;
                ImGui::TableSetColumnIndex(col);
                ImGui::Text("r%d: 0x%08X", reg, regs[reg]);
            }
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("cpsr: 0x%08X", m_debugger->view_cpsr());
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("spsr: 0x%08X", m_debugger->view_psr());

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("ie: 0x%08X", m_debugger->view_ie());
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("if: 0x%08X", m_debugger->view_if());

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("pipeline: 0x%08X", m_debugger->view_pipeline());
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("flush?: %s", m_debugger->is_pipeline_invalid() ? "YES" : "NO");

        ImGui::EndTable();
    }

    ImGui::End();
}

void Window::render_breakpoint_modal() {
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5, 0.5));

    if (ImGui::BeginPopupModal("Set Breakpoint", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        static char input[128] = "";
        ImGui::InputTextWithHint("##BREAKPOINT_TEXT_INPUT", "0xXXXXXXXX", input, 128);
        ImGui::Spacing();
        if (ImGui::Button("Set", ImVec2(60, 0))) {
            std::uint32_t breakpoint = std::stoul(std::string(input), nullptr, 16);
            m_breakpoint = breakpoint;
            m_breakpoint_reached = false;
            m_cpu->reset();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(60, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
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

        if (m_menu_bar.m_toggle_debug_panel) render_debug_window(); // TODO: resume automatically when closed?
        if (m_menu_bar.m_toggle_demo_window) ImGui::ShowDemoWindow(&m_menu_bar.m_toggle_demo_window);

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
