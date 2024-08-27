add_subdirectory("core")
add_subdirectory("libs/imgui")
add_executable(${PROJECT_NAME} 
    main.cpp 
    debugger.cpp 
    window.cpp 
    program_options.cpp
)
find_package(SDL2 REQUIRED COMPONENTS SDL2)
target_link_libraries(${PROJECT_NAME} PRIVATE imgui core SDL2::SDL2)
target_link_libraries(imgui PRIVATE SDL2::SDL2)