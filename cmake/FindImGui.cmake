# FindImGui.cmake
#
# Finds ImGui library
#
# This will define the following variables
# IMGUI_FOUND
# IMGUI_INCLUDE_DIRS
#
# and the following imported targets
#
# ImGui::ImGui - ImGui core library

list(APPEND IMGUI_SEARCH_PATH
    ${IMGUI_ROOT}
    $ENV{IMGUI_ROOT}
    )

find_path(IMGUI_INCLUDE_DIR
      NAMES imgui.cpp imgui_draw.cpp imgui_demo.cpp
      PATHS ${IMGUI_SEARCH_PATH})

if(NOT IMGUI_INCLUDE_DIR)
    message(FATAL_ERROR "IMGUI imgui.cpp not found. Set IMGUI_ROOT to imgui's top-level path (containing \"imgui.cpp\" and \"imgui.h\" files).\n")
endif()

message(STATUS "Found imgui.cpp in ${IMGUI_INCLUDE_DIR}")
set(IMGUI_FOUND TRUE)

add_library(ImGui
    ${IMGUI_INCLUDE_DIR}/imgui.cpp
    ${IMGUI_INCLUDE_DIR}/imgui_draw.cpp)

add_library(ImGui_Demo
    ${IMGUI_INCLUDE_DIR}/imgui_demo.cpp)

target_link_libraries(ImGui_Demo
    PUBLIC ImGui)

add_library(ImGui::ImGui ALIAS ImGui)
add_library(ImGui::Demo ALIAS ImGui_Demo)
