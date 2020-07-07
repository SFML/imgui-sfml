cmake_minimum_required(VERSION 3.1)

project(imgui_sfml
  LANGUAGES CXX
  VERSION 2.1
)

# In CMake 3.12+ this policy will automatically take the ImGui_ROOT and SFML_ROOT environment variables
# into account as hints for the find_package calls.
if(POLICY CMP0074)
  cmake_policy(SET CMP0074 NEW)
endif()
# In CMake 3.13+ this policy enables you to define normal variables with option names as if you defined
# these options. Useful for add_subdirectory(ImGui-SFML)
if(POLICY CMP0077)
  cmake_policy(SET CMP0077 NEW)
endif()

option(IMGUI_SFML_BUILD_EXAMPLES "Build ImGui_SFML examples" OFF)
option(IMGUI_SFML_FIND_SFML "Use find_package to find SFML" ON)
option(IMGUI_SFML_IMGUI_DEMO "Build imgui_demo.cpp" OFF)

# If you want to use your own user config when compiling ImGui, please set the following variables
# For example, if you have your config in /path/to/dir/with/config/myconfig.h, set the variables as follows:
#
#   IMGUI_SFML_USE_DEFAULT_CONFIG = OFF
#   IMGUI_SFML_CONFIG_DIR = /path/to/dir/with/config
#   IMGUI_SFML_CONFIG_NAME = "myconfig.h"
#
# If you set IMGUI_SFML_CONFIG_INSTALL_DIR, ImGui-SFML won't install your custom config, because
# you might want to do it yourself
option(IMGUI_SFML_USE_DEFAULT_CONFIG "Use default imconfig-SFML.h" ON)
set(IMGUI_SFML_CONFIG_DIR "${CMAKE_CURRENT_LIST_DIR}" CACHE PATH "Path to a directory containing user ImGui config")
set(IMGUI_SFML_CONFIG_NAME "imconfig-SFML.h" CACHE STRING "Name of a custom user ImGui config header")
set(IMGUI_SFML_CONFIG_INSTALL_DIR "" CACHE PATH "Path where user's config header will be installed")

# For FindImGui.cmake
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

if (IMGUI_SFML_FIND_SFML)
	if (NOT BUILD_SHARED_LIBS)
		set(SFML_STATIC_LIBRARIES ON)
	endif()
	find_package(SFML 2.5 COMPONENTS graphics system window)

	if(NOT SFML_FOUND)
    message(FATAL_ERROR "SFML 2 directory not found. Set SFML_DIR to directory where SFML was built (or one which ccontains SFMLConfig.cmake)")
	endif()
endif()

# ImGui does not provide native support for CMakeLists, workaround for now to have
# users specify IMGUI_DIR. Waiting for this PR to get merged...
#    https://github.com/ocornut/imgui/pull/1713
if(NOT IMGUI_DIR)
  set(IMGUI_DIR "" CACHE PATH "imgui top-level directory")
  message(FATAL_ERROR "ImGui directory not found. Set IMGUI_DIR to imgui's top-level path (containing 'imgui.h' and other files).\n")
endif()

# This uses FindImGui.cmake provided in ImGui-SFML repo for now
find_package(ImGui 1.68 REQUIRED)

# these headers will be installed alongside ImGui-SFML
set(IMGUI_PUBLIC_HEADERS
  ${IMGUI_INCLUDE_DIR}/imconfig.h
  ${IMGUI_INCLUDE_DIR}/imgui.h
  ${IMGUI_INCLUDE_DIR}/imgui_internal.h # not actually public, but users might need it
  ${IMGUI_INCLUDE_DIR}/imstb_rectpack.h
  ${IMGUI_INCLUDE_DIR}/imstb_textedit.h
  ${IMGUI_INCLUDE_DIR}/imstb_truetype.h
  ${IMGUI_INCLUDE_DIR}/misc/cpp/imgui_stdlib.h
)

if (IMGUI_SFML_IMGUI_DEMO)
    list(APPEND IMGUI_SOURCES ${IMGUI_DEMO_SOURCES})
endif()

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

# Add pretty alias
add_library(ImGui-SFML::ImGui-SFML ALIAS ImGui-SFML)

target_link_libraries(ImGui-SFML
  PUBLIC
    sfml-graphics
    sfml-system
    sfml-window
    ${OPENGL_LIBRARIES}
)

include(GNUInstallDirs)

target_include_directories(ImGui-SFML
  PUBLIC
    $<BUILD_INTERFACE:${IMGUI_INCLUDE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

if(NOT IMGUI_SFML_USE_DEFAULT_CONFIG)
  if (IMGUI_SFML_CONFIG_DIR)
    target_include_directories(ImGui-SFML
      PUBLIC
        $<BUILD_INTERFACE:${IMGUI_SFML_CONFIG_DIR}>
    )

    if (IMGUI_SFML_CONFIG_INSTALL_DIR)
      target_include_directories(ImGui-SFML
        PUBLIC
          $<INSTALL_INTERFACE:${IMGUI_SFML_CONFIG_INSTALL_DIR}>
      )
    endif()
  else()
    message(FATAL_ERROR "IMGUI_SFML_CONFIG_DIR should be set if IMGUI_SFML_USE_DEFAULT_CONFIG is OFF")
  endif()
endif()

target_compile_definitions(ImGui-SFML
  PUBLIC
    IMGUI_USER_CONFIG="${IMGUI_SFML_CONFIG_NAME}"
)

if(BUILD_SHARED_LIBS)
  target_compile_definitions(ImGui-SFML PRIVATE IMGUI_SFML_SHARED_LIB)
  set_target_properties(ImGui-SFML PROPERTIES
    DEFINE_SYMBOL "IMGUI_SFML_EXPORTS"
  )
  set_target_properties(ImGui-SFML PROPERTIES
    DEBUG_POSTFIX "_d"
  )
endif()

set(IMGUI_SFML_PUBLIC_HEADERS
  "${CMAKE_CURRENT_LIST_DIR}/imgui-SFML.h"
  "${CMAKE_CURRENT_LIST_DIR}/imgui-SFML_export.h"
)

if (IMGUI_SFML_USE_DEFAULT_CONFIG OR
    (NOT DEFINED "${IMGUI_SFML_CONFIG_INSTALL_DIR}"))
  list(APPEND IMGUI_SFML_PUBLIC_HEADERS
    "${IMGUI_SFML_CONFIG_DIR}/${IMGUI_SFML_CONFIG_NAME}"
  )
# If user set IMGUI_SFML_CONFIG_INSTALL_DIR, it means that they'll install file themselves
endif()

list(APPEND IMGUI_SFML_PUBLIC_HEADERS "${IMGUI_PUBLIC_HEADERS}")

set_target_properties(ImGui-SFML PROPERTIES
  PUBLIC_HEADER "${IMGUI_SFML_PUBLIC_HEADERS}"
)

if(IMGUI_SFML_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

# installation rules
install(TARGETS ImGui-SFML
  EXPORT ImGui-SFML
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT ImGui-SFML
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ImGui-SFML
  NAMESPACE ImGui-SFML::
  FILE ImGui-SFMLConfig.cmake
)
