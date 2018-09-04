# Only tested in 3.1, may work in older versions.
cmake_minimum_required(VERSION 3.1)

project(imgui_sfml_example)

add_executable(imgui_sfml_example main.cpp)

target_compile_definitions(imgui_sfml_example PRIVATE IMGUI_INCLUDE_IMCONFIG_H)
# FIXME(SMA) : Wrong header used here when IMGUI_ROOT contains a imconfig.h
target_include_directories(imgui_sfml_example PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR} # needed for imgui.h to find our imconfig.h
    )

target_link_libraries(imgui_sfml_example ImGui-SFML)
install(TARGETS imgui_sfml_example DESTINATION bin)
set_target_properties(imgui_sfml_example PROPERTIES DEBUG_POSTFIX _d)
