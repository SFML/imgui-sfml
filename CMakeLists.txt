cmake_minimum_required(VERSION 3.1)

project(imgui_sfml
  LANGUAGES CXX
  VERSION 2.0
)

option(IMGUI_SFML_BUILD_EXAMPLES "Build ImGui_SFML examples" OFF)
option(IMGUI_SFML_FIND_SFML "Use find_package to find SFML" ON)

# Find required libraries
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

if (IMGUI_SFML_FIND_SFML)
	find_package(SFML 2.3 COMPONENTS graphics system window)

	if(NOT SFML_FOUND)
		set(SFML_ROOT "" CACHE PATH "SFML top-level directory")
		message(FATAL_ERROR "SFML 2 directory not found. Set SFML_ROOT to SFML's top-level path (containing \"include\" and \"lib\" directories).\n")
	endif()
endif()

# ImGui does not provide native support for CMakeLists, workaround for now to have
# users specify IMGUI_ROOT. Waiting for this PR to get merged...
#    https://github.com/ocornut/imgui/pull/1713
if(NOT IMGUI_ROOT)
  set(IMGUI_ROOT "" CACHE PATH "imgui top-level directory")
  message(FATAL_ERROR "ImGui directory not found. Set IMGUI_ROOT to imgui's top-level path (containing 'imgui.h' and other files).\n")
endif()

find_package(ImGui REQUIRED)

# CMake 3.11 and later prefer to choose GLVND, but we choose legacy OpenGL just because it's safer
# (unless the OpenGL_GL_PREFERENCE was explicitly set)
# See CMP0072 for more details (cmake --help-policy CMP0072)
if ((NOT ${CMAKE_VERSION} VERSION_LESS 3.11) AND (NOT OpenGL_GL_PREFERENCE))
    set(OpenGL_GL_PREFERENCE "LEGACY")
endif()

find_package(OpenGL REQUIRED)

add_library(ImGui-SFML
  imgui-SFML.cpp
	${IMGUI_SOURCES}
)

# TODO: this might not be needed as CMake sets target_name_EXPORTS with some subsitutions
# So we need to test if compiling as shared library works
set_target_properties(ImGui-SFML PROPERTIES
  DEFINE_SYMBOL "IMGUI_SFML_EXPORTS"
)

target_link_libraries(ImGui-SFML
  PUBLIC
    sfml-graphics
    sfml-system
    sfml-window
    ${OPENGL_LIBRARIES}
)

target_include_directories(ImGui-SFML
  PUBLIC
    ${IMGUI_INCLUDE_DIR}
    ${CMAKE_CURRENT_LIST_DIR}
)

target_compile_definitions(ImGui-SFML
  PUBLIC
    IMGUI_USER_CONFIG="imconfig-SFML.h"
    IMGUI_DISABLE_INCLUDE_IMCONFIG_H)

# Add pretty alias
add_library(ImGui-SFML::ImGui-SFML ALIAS ImGui-SFML)

if(IMGUI_SFML_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

# TODO: add install rules
