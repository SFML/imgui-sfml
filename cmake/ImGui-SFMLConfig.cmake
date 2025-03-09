include(CMakeFindDependencyMacro)
find_dependency(OpenGL)
find_dependency(SFML COMPONENTS Graphics)

include(${CMAKE_CURRENT_LIST_DIR}/ImGui-SFML.cmake)
