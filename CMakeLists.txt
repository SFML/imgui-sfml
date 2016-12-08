# Only tested in 3.1, may work in older versions.
# .
# Install SFML or set SFML_ROOT and set IMGUI_ROOT prior to running cmake
# .
# Provides the follow variables:
# IMGUI_SFML_INCLUDE_DIRS - imgui and imgui_sfml include paths
# IMGUI_SOURCES - imgui.cpp source path to link with your binary
# IMGUI_SFML_SOURCES - imgui_sfml.cpp source path to link with your binary
# IMGUI_SFML_DEPENDENCIES - found dependencies to link with your library (sfml)
# .
# Sample usage:
#   add_subdirectory(repos/imgui-sfml)
#   include_directories("${IMGUI_SFML_INCLUDE_DIRS}")
#   add_executable(MY_PROJECT ${IMGUI_SOURCES} ${IMGUI_SFML_SOURCES} ${SRCS})
#   ...
#   target_link_libraries(MY_PROJECT ${IMGUI_SFML_DEPENDENCIES})
# .
cmake_minimum_required(VERSION 3.1)

project(imgui_sfml)

option(IMGUI_SFML_BUILD_EXAMPLES "Build ImGui_SFML examples" ON)

# Find required libraries
find_package(SFML 2.2 COMPONENTS graphics window system)
message(status "** SFML Include: ${SFML_INCLUDE_DIR}")
message(status "** SFML Libraries: ${SFML_LIBRARIES}")
if(NOT SFML_FOUND)
    set(SFML_ROOT "" CACHE PATH "SFML top-level directory")
    message("---> SFML 2 directory not found. Set SFML_ROOT to SFML's top-level path (containing \"include\" and \"lib\" directories).\n")
endif()

# ImGui does not provide native support for CMakeLists, workaround for now to have
# users specify IMGUI_ROOT. See:
#    https://github.com/ocornut/imgui/pull/255
if(NOT IMGUI_ROOT)
    set(IMGUI_ROOT "" CACHE PATH "imgui top-level directory")
    message("---> ImGui directory not found. Set IMGUI_ROOT to imgui's top-level path (containing \"imgui.cpp\" and \"imgui.h\" files).\n")
endif()

# Do a pseudo find files for ImGui once IMGUI_ROOT is set
if(IMGUI_ROOT)
    set(IMGUI_SERACH_PATH
        ${IMGUI_ROOT}
        $ENV{IMGUI_ROOT}
        )
    find_path(IMGUI_INCLUDE_DIR imgui.cpp
              PATHS ${IMGUI_SERACH_PATH})
    if(NOT IMGUI_INCLUDE_DIR)
        message(FATAL_ERROR "---> IMGUI imgui.cpp not found. Set IMGUI_ROOT to imgui's top-level path (containing \"imgui.cpp\" and \"imgui.h\" files).\n")
    else()
        file(GLOB IMGUI_FILES
            "${IMGUI_INCLUDE_DIR}/imgui_draw.cpp"
            "${IMGUI_INCLUDE_DIR}/imgui_demo.cpp"
            "${IMGUI_INCLUDE_DIR}/imgui.cpp" )
        message("Found imgui.cpp in ${IMGUI_INCLUDE_DIR}")
        # Rename that pesky imconfig.h file for the user.
        install(FILES ${IMGUI_INCLUDE_DIR}/imconfig.h DESTINATION include RENAME imconfig-sample.h)
    endif()
endif()

# Glob up both source and headers as sources for VS users.
file(GLOB IMGUI_SFML_FILES "${PROJECT_SOURCE_DIR}/*.cpp" "${PROJECT_SOURCE_DIR}/*.h")

# Set these for users to use
set(IMGUI_SFML_INCLUDE_DIRS
    ${PROJECT_SOURCE_DIR}
    ${IMGUI_INCLUDE_DIR}
    ${SFML_INCLUDE_DIR})

set(IMGUI_SOURCES
    ${IMGUI_FILES})

set(IMGUI_SFML_SOURCES
    ${IMGUI_SFML_FILES})

set(IMGUI_SFML_DEPENDENCIES
    ${SFML_DEPENDENCIES}
    ${SFML_LIBRARIES})

if(IMGUI_SFML_BUILD_EXAMPLES)
  # Build examples
  add_subdirectory(examples)
endif(IMGUI_SFML_BUILD_EXAMPLES)
