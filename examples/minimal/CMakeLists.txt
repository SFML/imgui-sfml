add_executable(imgui_sfml_example_minimal main.cpp)
target_link_libraries(imgui_sfml_example_minimal PRIVATE ImGui-SFML::ImGui-SFML)
target_compile_options(imgui_sfml_example_minimal PRIVATE ${IMGUI_SFML_WARNINGS})
