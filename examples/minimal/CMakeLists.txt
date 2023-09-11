add_executable(imgui_sfml_example_minimal
  main.cpp
  ${IMGUI_DEMO_SOURCES}
)

target_link_libraries(imgui_sfml_example_minimal PRIVATE ImGui-SFML::ImGui-SFML)
