cmake_minimum_required(VERSION 3.22)
project(imgui_sfml VERSION 3.0.0 LANGUAGES CXX)

option(IMGUI_SFML_FIND_SFML "Use find_package to find SFML" ON)
option(IMGUI_SFML_ENABLE_WARNINGS "Enable compiler warnings" OFF)
option(IMGUI_SFML_DISABLE_OBSOLETE_FUNCTIONS "Disable obsolete ImGui functions" OFF)

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
set(IMGUI_SFML_CONFIG_DIR ${PROJECT_SOURCE_DIR} CACHE PATH "Path to a directory containing user ImGui config")
set(IMGUI_SFML_CONFIG_NAME imconfig-SFML.h CACHE STRING "Name of a custom user ImGui config header")
set(IMGUI_SFML_CONFIG_INSTALL_DIR "" CACHE PATH "Path where user's config header will be installed")

# For FindImGui.cmake
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

if(IMGUI_SFML_FIND_SFML)
  if(NOT BUILD_SHARED_LIBS)
    set(SFML_STATIC_LIBRARIES ON)
  endif()
  find_package(SFML 3 REQUIRED COMPONENTS Graphics)
endif()

# ImGui does not provide native support for CMakeLists, workaround for now to have
# users specify IMGUI_DIR. Waiting for this PR to get merged...
#    https://github.com/ocornut/imgui/pull/1713
if(NOT IMGUI_DIR)
  set(IMGUI_DIR "" CACHE PATH "imgui top-level directory")
  message(FATAL_ERROR "ImGui directory not found. Set IMGUI_DIR to imgui's top-level path (containing 'imgui.h' and other files).\n")
endif()

# This uses FindImGui.cmake provided in ImGui-SFML repo for now
find_package(ImGui 1.91.1 REQUIRED)

# These headers will be installed alongside ImGui-SFML
set(IMGUI_PUBLIC_HEADERS
  ${IMGUI_INCLUDE_DIR}/imconfig.h
  ${IMGUI_INCLUDE_DIR}/imgui.h
  ${IMGUI_INCLUDE_DIR}/imgui_internal.h # not actually public, but users might need it
  ${IMGUI_INCLUDE_DIR}/imstb_rectpack.h
  ${IMGUI_INCLUDE_DIR}/imstb_textedit.h
  ${IMGUI_INCLUDE_DIR}/imstb_truetype.h
  ${IMGUI_INCLUDE_DIR}/misc/cpp/imgui_stdlib.h
)

# CMake 3.11 and later prefer to choose GLVND, but we choose legacy OpenGL just because it's safer
# (unless the OpenGL_GL_PREFERENCE was explicitly set)
# See CMP0072 for more details (cmake --help-policy CMP0072)
if(NOT OpenGL_GL_PREFERENCE)
  set(OpenGL_GL_PREFERENCE "LEGACY")
endif()

find_package(OpenGL REQUIRED)

include(GNUInstallDirs)

# Define ImGui-SFML
add_library(ImGui-SFML imgui-SFML.cpp ${IMGUI_SOURCES})
add_library(ImGui-SFML::ImGui-SFML ALIAS ImGui-SFML)
target_include_directories(ImGui-SFML PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_include_directories(ImGui-SFML SYSTEM PUBLIC
  $<BUILD_INTERFACE:${IMGUI_INCLUDE_DIR}>
)
target_link_libraries(ImGui-SFML PUBLIC SFML::Graphics OpenGL::GL)
if(WIN32 AND MINGW)
  target_link_libraries(ImGui-SFML PUBLIC imm32)
endif()
if(BUILD_SHARED_LIBS)
  target_compile_definitions(ImGui-SFML PUBLIC IMGUI_SFML_SHARED_LIB
                                        PRIVATE IMGUI_SFML_EXPORTS)
  set_target_properties(ImGui-SFML PROPERTIES DEBUG_POSTFIX "_d")
endif()
if(IMGUI_SFML_DISABLE_OBSOLETE_FUNCTIONS)
  target_compile_definitions(ImGui-SFML PUBLIC IMGUI_DISABLE_OBSOLETE_FUNCTIONS)
endif()

# Add compiler warnings
if(IMGUI_SFML_ENABLE_WARNINGS)
  if(MSVC)
    set(IMGUI_SFML_WARNINGS /WX /W4 /permissive-)
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "(GNU|Clang)")
    set(IMGUI_SFML_WARNINGS -Werror -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion)
  endif()
endif()
foreach(WARNING ${IMGUI_SFML_WARNINGS})
  set_property(SOURCE imgui-SFML.cpp APPEND_STRING PROPERTY COMPILE_FLAGS " ${WARNING}")
endforeach()

# Specify ImGui user config
if(NOT IMGUI_SFML_USE_DEFAULT_CONFIG)
  if(IMGUI_SFML_CONFIG_DIR)
    target_include_directories(ImGui-SFML PUBLIC $<BUILD_INTERFACE:${IMGUI_SFML_CONFIG_DIR}>)

    if(IMGUI_SFML_CONFIG_INSTALL_DIR)
      target_include_directories(ImGui-SFML PUBLIC $<INSTALL_INTERFACE:${IMGUI_SFML_CONFIG_INSTALL_DIR}>)
    endif()
  else()
    message(FATAL_ERROR "IMGUI_SFML_CONFIG_DIR should be set if IMGUI_SFML_USE_DEFAULT_CONFIG is OFF")
  endif()
endif()
target_compile_definitions(ImGui-SFML PUBLIC IMGUI_USER_CONFIG="${IMGUI_SFML_CONFIG_NAME}")

# Collect public headers
set(IMGUI_SFML_PUBLIC_HEADERS
  ${PROJECT_SOURCE_DIR}/imgui-SFML.h
  ${PROJECT_SOURCE_DIR}/imgui-SFML_export.h
  ${IMGUI_PUBLIC_HEADERS}
)
if(IMGUI_SFML_USE_DEFAULT_CONFIG OR (NOT DEFINED "${IMGUI_SFML_CONFIG_INSTALL_DIR}"))
  list(APPEND IMGUI_SFML_PUBLIC_HEADERS "${IMGUI_SFML_CONFIG_DIR}/${IMGUI_SFML_CONFIG_NAME}")
  # If user set IMGUI_SFML_CONFIG_INSTALL_DIR, it means that they'll install file themselves
endif()
set_target_properties(ImGui-SFML PROPERTIES PUBLIC_HEADER "${IMGUI_SFML_PUBLIC_HEADERS}")

# Installation rules
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
)

install(FILES ${PROJECT_SOURCE_DIR}/cmake/ImGui-SFMLConfig.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ImGui-SFML
)

# Stop configuration if being consumed by a higher level project
if(NOT PROJECT_IS_TOP_LEVEL)
  return()
endif()

option(IMGUI_SFML_BUILD_EXAMPLES "Build ImGui-SFML examples" OFF)
if(IMGUI_SFML_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

option(IMGUI_SFML_BUILD_TESTING "Build ImGui-SFML tests" OFF)
if(IMGUI_SFML_BUILD_TESTING)
  enable_testing()
  add_subdirectory(tests)
endif()

add_custom_target(tidy
  COMMAND run-clang-tidy -quiet -p ${CMAKE_BINARY_DIR} *.cpp examples/**/*.cpp tests/*.cpp
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
