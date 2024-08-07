cmake_minimum_required(VERSION 3.29.3)
project(gba)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-Wall -Werror -Wold-style-cast")

add_executable(${PROJECT_NAME} src/main.cpp src/program_options.cpp src/cpu.cpp src/memory.cpp src/ppu.cpp)
