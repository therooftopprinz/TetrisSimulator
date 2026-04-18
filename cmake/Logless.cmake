# Logless logging library: use the git submodule when present, otherwise fetch at configure time.

set(TETRIS_LOGLESS_GIT_REPOSITORY "https://github.com/therooftopprinz/Logless.git"
  CACHE STRING "Git repository for Logless when the submodule is not checked out")
set(TETRIS_LOGLESS_GIT_TAG "master"
  CACHE STRING "Git branch or tag for Logless when fetching (ignored when using the submodule)")

if(EXISTS "${CMAKE_SOURCE_DIR}/Logless/CMakeLists.txt")
  add_subdirectory("${CMAKE_SOURCE_DIR}/Logless" "${CMAKE_BINARY_DIR}/logless-build" EXCLUDE_FROM_ALL)
else()
  include(FetchContent)
  FetchContent_Declare(
    logless_fc
    GIT_REPOSITORY "${TETRIS_LOGLESS_GIT_REPOSITORY}"
    GIT_TAG "${TETRIS_LOGLESS_GIT_TAG}"
    GIT_SHALLOW TRUE
  )
  FetchContent_GetProperties(logless_fc)
  if(NOT logless_fc_POPULATED)
    FetchContent_Populate(logless_fc)
  endif()
  # EXCLUDE_FROM_ALL: avoid building Logless's optional test binary under this project's -Werror.
  add_subdirectory("${logless_fc_SOURCE_DIR}" "${logless_fc_BINARY_DIR}" EXCLUDE_FROM_ALL)
endif()

if(NOT TARGET logless)
  message(FATAL_ERROR
    "Logless did not define target \"logless\". "
    "If you use a submodule, run: git submodule update --init --recursive")
endif()
