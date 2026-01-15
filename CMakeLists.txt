cmake_minimum_required(VERSION 3.16)
project(game)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(extern)

set(GAME_SRCS
    "src/main.cpp"
    "src/device.cpp"
    "src/math.cpp"
)

if(WIN32)
    set(GAME_SRCS ${GAME_SRCS} "src/impl/device_win.cpp")
else()
    set(GAME_SRCS ${GAME_SRCS} "src/impl/device_r36s.cpp")
endif()

add_executable(game ${GAME_SRCS})
target_include_directories(game PRIVATE "include")
target_link_libraries(game PRIVATE extern_deps)
target_compile_definitions(game PRIVATE ASSETS_PATH="${CMAKE_CURRENT_SOURCE_DIR}/assets")