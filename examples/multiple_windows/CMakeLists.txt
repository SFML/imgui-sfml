add_executable(imgui_sfml_example_multiple_windows
  main.cpp
  ${IMGUI_DEMO_SOURCES}
)

target_link_libraries(imgui_sfml_example_multiple_windows PRIVATE ImGui-SFML::ImGui-SFML)
