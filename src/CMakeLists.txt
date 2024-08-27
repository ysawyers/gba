add_subdirectory("libs/imgui")
add_executable(${PROJECT_NAME} 
    main.cpp 
    cpu.cpp 
    debugger.cpp 
    window.cpp 
    ppu.cpp 
    memory.cpp 
    program_options.cpp
    thumb.cpp
)
find_package(SDL2 REQUIRED COMPONENTS SDL2)
target_link_libraries(${PROJECT_NAME} PRIVATE imgui SDL2::SDL2)
target_link_libraries(imgui PRIVATE SDL2::SDL2)