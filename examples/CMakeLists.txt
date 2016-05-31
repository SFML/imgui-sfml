# Only tested in 3.1, may work in older versions.
cmake_minimum_required(VERSION 3.1)

find_package(OpenGL REQUIRED)

project(imgui_sfml_example)

file(TO_CMAKE_PATH "${PROJECT_SOURCE_DIR}/main.cpp" EXAMPLE_FILES)
set(SRCS "imconfig.h" ${IMGUI_SOURCES} ${IMGUI_SFML_SOURCES} ${EXAMPLE_FILES})

add_executable(imgui_sfml_example ${SRCS})

target_compile_definitions(imgui_sfml_example PRIVATE IMGUI_INCLUDE_IMCONFIG_H)
# FIXME(SMA) : Wrong header used here when IMGUI_ROOT contains a imconfig.h
target_include_directories(imgui_sfml_example PRIVATE
    ${PROJECT_SOURCE_DIR} # needed for imgui.h to find our imconfig.h
    ${IMGUI_SFML_INCLUDE_DIRS}
    )

target_link_libraries(imgui_sfml_example ${OPENGL_LIBRARIES} ${IMGUI_SFML_DEPENDENCIES})
install(TARGETS imgui_sfml_example DESTINATION bin)
set_target_properties(imgui_sfml_example PROPERTIES DEBUG_POSTFIX _d)
