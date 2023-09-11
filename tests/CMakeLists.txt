# Get Catch2
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})
find_package(Catch2 3.4.0 REQUIRED)
include(Catch)

# Test library
add_executable(test-imgui-sfml imconfig-SFML.cpp)
target_link_libraries(test-imgui-sfml PRIVATE ImGui-SFML::ImGui-SFML Catch2::Catch2WithMain)
target_compile_options(test-imgui-sfml PRIVATE ${IMGUI_SFML_WARNINGS})
catch_discover_tests(test-imgui-sfml)
