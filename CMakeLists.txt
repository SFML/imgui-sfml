cmake_minimum_required(VERSION 3.1)

project(imgui_sfml)

option(IMGUI_SFML_BUILD_EXAMPLES "Build ImGui_SFML examples" ON)
option(IMGUI_SFML_FIND_SFML "Use find_package to find SFML" ON)

# Find required libraries
if (IMGUI_SFML_FIND_SFML)
	find_package(SFML 2.5 COMPONENTS graphics)

	if(NOT SFML_FOUND)
		set(SFML_ROOT "" CACHE PATH "SFML top-level directory")
		message(FATAL_ERROR "SFML 2 directory not found. Set SFML_ROOT to SFML's top-level path (containing \"include\" and \"lib\" directories).\n")
	endif()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# ImGui does not provide native support for CMakeLists, workaround for now to have
# users specify IMGUI_ROOT. See:
#    https://github.com/ocornut/imgui/pull/255
if(NOT IMGUI_ROOT)
    set(IMGUI_ROOT "" CACHE PATH "imgui top-level directory")
    message(FATAL_ERROR "ImGui directory not found. Set IMGUI_ROOT to imgui's top-level path (containing \"imgui.cpp\" and \"imgui.h\" files).\n")
endif()

find_package(ImGui REQUIRED)

find_package(OpenGL REQUIRED)

add_library(ImGui-SFML
    ${CMAKE_CURRENT_LIST_DIR}/imgui-SFML.cpp
	${IMGUI_SOURCES})

set_target_properties(ImGui-SFML PROPERTIES
  DEFINE_SYMBOL "IMGUI_SFML_EXPORTS"
)

target_link_libraries(ImGui-SFML
    PUBLIC
        sfml-graphics
        ${OPENGL_LIBRARIES}
)

target_include_directories(ImGui-SFML
    PUBLIC
	    ${IMGUI_INCLUDE_DIR}
        ${CMAKE_CURRENT_LIST_DIR}
)

target_compile_definitions(ImGui-SFML PUBLIC IMGUI_USER_CONFIG="imconfig-SFML.h")

add_library(ImGui-SFML::ImGui-SFML ALIAS ImGui-SFML)
