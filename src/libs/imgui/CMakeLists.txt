add_library(imgui 
    SHARED 
    imgui_draw.cpp 
    imgui_tables.cpp 
    imgui_widgets.cpp 
    imgui_demo.cpp
    imgui_impl_sdlrenderer2.cpp
    imgui_impl_sdl2.cpp 
    imgui.cpp
)
target_compile_options(imgui PRIVATE "-w")