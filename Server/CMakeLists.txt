cmake_minimum_required(VERSION 3.22)
project(Snake)

set(CMAKE_CXX_STANDARD 20)

include_directories(libs/imgui/ headers/ libs/include/)

add_executable(Snake libs/imgui/backends/imgui_impl_opengl3.cpp libs/imgui/imgui.cpp libs/imgui/imgui_widgets.cpp libs/imgui/imgui_tables.cpp libs/imgui/imgui_draw.cpp libs/imgui/imgui_demo.cpp libs/imgui/backends/imgui_impl_glfw.cpp src/main.cpp src/Game.cpp headers/Game.h src/DummyPlayer.cpp headers/DummyPlayer.h headers/Player.h headers/utils.h src/Snake.cpp headers/Snake.h src/render/GameDisplay.cpp headers/render/GameDisplay.h libs/glad/glad.c src/GameCreator.cpp headers/GameCreator.h src/render/ImGuiRenderer.cpp headers/render/ImGuiRenderer.h headers/network/snake_network.h src/network/snake_network.cpp)

target_link_libraries(Snake ${CMAKE_CURRENT_SOURCE_DIR}/libs/lib/glfw3.lib)