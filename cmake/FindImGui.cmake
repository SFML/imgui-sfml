# FindImGui.cmake
#
# Finds ImGui library
#
# This will define the following variables
# IMGUI_FOUND
# IMGUI_INCLUDE_DIRS
# IMGUI_SOURCES
# IMGUI_DEMO_SOURCES
# IMGUI_VERSION

list(APPEND IMGUI_SEARCH_PATH
  ${IMGUI_DIR}
)

find_path(IMGUI_INCLUDE_DIR
  NAMES imgui.h
  PATHS ${IMGUI_SEARCH_PATH}
  NO_DEFAULT_PATH
)

if(NOT IMGUI_INCLUDE_DIR)
  message(FATAL_ERROR "IMGUI imgui.cpp not found. Set IMGUI_DIR to imgui's top-level path (containing \"imgui.cpp\" and \"imgui.h\" files).\n")
endif()

set(IMGUI_SOURCES
  ${IMGUI_INCLUDE_DIR}/imgui.cpp
  ${IMGUI_INCLUDE_DIR}/imgui_draw.cpp
  ${IMGUI_INCLUDE_DIR}/imgui_widgets.cpp
  ${IMGUI_INCLUDE_DIR}/misc/cpp/imgui_stdlib.cpp
)

set(IMGUI_DEMO_SOURCES
  ${IMGUI_INCLUDE_DIR}/imgui_demo.cpp
)

# Extract version from header
file(
  STRINGS
  ${IMGUI_INCLUDE_DIR}/imgui.h
  IMGUI_VERSION
  REGEX "#define IMGUI_VERSION "
)

if(NOT IMGUI_VERSION)
  message(SEND_ERROR "Can't find version number in ${IMGUI_INCLUDE_DIR}/imgui.h.")
endif()
# Transform '#define IMGUI_VERSION "X.Y"' into 'X.Y'
string(REGEX REPLACE ".*\"(.*)\".*" "\\1" IMGUI_VERSION "${IMGUI_VERSION}")

# Check required version
if(${IMGUI_VERSION} VERSION_LESS ${ImGui_FIND_VERSION})
  set(IMGUI_FOUND FALSE)
  message(FATAL_ERROR "ImGui at with at least v${ImGui_FIND_VERSION} was requested, but only v${IMGUI_VERSION} was found")
else()
  set(IMGUI_FOUND TRUE)
  message(STATUS "Found ImGui v${IMGUI_VERSION} in ${IMGUI_INCLUDE_DIR}")
endif()
