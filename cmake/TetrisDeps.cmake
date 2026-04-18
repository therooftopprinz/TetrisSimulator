# Third-party sources: optional git submodule sync, then Logless (CMake) + header-only BFC/cum (FetchContent).

option(TETRIS_INIT_GIT_SUBMODULES
  "Run `git submodule update --init --recursive` at configure time when .git is present"
  ON)

if(TETRIS_INIT_GIT_SUBMODULES AND EXISTS "${CMAKE_SOURCE_DIR}/.git" AND EXISTS "${CMAKE_SOURCE_DIR}/.gitmodules")
  find_package(Git QUIET)
  if(GIT_FOUND)
    execute_process(
      COMMAND "${GIT_EXECUTABLE}" submodule update --init --recursive
      WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
      RESULT_VARIABLE _tetris_git_submod_rc
    )
    if(NOT _tetris_git_submod_rc EQUAL 0)
      message(WARNING
        "git submodule update failed (exit ${_tetris_git_submod_rc}); "
        "continuing — missing trees may be fetched below.")
    endif()
  endif()
endif()

set(TETRIS_BFC_ROOT "${CMAKE_SOURCE_DIR}/BFC" CACHE PATH "Root of the BFC headers tree")
set(TETRIS_CUM_ROOT "${CMAKE_SOURCE_DIR}/cum" CACHE PATH "Root of the cum headers tree")

set(TETRIS_BFC_GIT_REPOSITORY "https://github.com/therooftopprinz/BFC.git" CACHE STRING "Git repo for BFC when missing")
set(TETRIS_BFC_GIT_TAG "master" CACHE STRING "Git ref for BFC when fetching")
set(TETRIS_CUM_GIT_REPOSITORY "https://github.com/therooftopprinz/cum.git" CACHE STRING "Git repo for cum when missing")
set(TETRIS_CUM_GIT_TAG "master" CACHE STRING "Git ref for cum when fetching")

if(NOT EXISTS "${TETRIS_BFC_ROOT}/src/bfc/function.hpp")
  include(FetchContent)
  FetchContent_Declare(
    tetris_bfc_fc
    GIT_REPOSITORY "${TETRIS_BFC_GIT_REPOSITORY}"
    GIT_TAG "${TETRIS_BFC_GIT_TAG}"
    GIT_SHALLOW TRUE
  )
  FetchContent_GetProperties(tetris_bfc_fc)
  if(NOT tetris_bfc_fc_POPULATED)
    FetchContent_Populate(tetris_bfc_fc)
  endif()
  set(TETRIS_BFC_ROOT "${tetris_bfc_fc_SOURCE_DIR}" CACHE PATH "Root of the BFC headers tree" FORCE)
endif()

if(NOT EXISTS "${TETRIS_CUM_ROOT}/src/cum/cum.hpp")
  include(FetchContent)
  FetchContent_Declare(
    tetris_cum_fc
    GIT_REPOSITORY "${TETRIS_CUM_GIT_REPOSITORY}"
    GIT_TAG "${TETRIS_CUM_GIT_TAG}"
    GIT_SHALLOW TRUE
  )
  FetchContent_GetProperties(tetris_cum_fc)
  if(NOT tetris_cum_fc_POPULATED)
    FetchContent_Populate(tetris_cum_fc)
  endif()
  set(TETRIS_CUM_ROOT "${tetris_cum_fc_SOURCE_DIR}" CACHE PATH "Root of the cum headers tree" FORCE)
endif()

if(NOT EXISTS "${TETRIS_BFC_ROOT}/src/bfc/function.hpp")
  message(FATAL_ERROR
    "BFC headers not found (expected ${TETRIS_BFC_ROOT}/src/bfc/function.hpp). "
    "Set TETRIS_BFC_ROOT or fix TETRIS_BFC_GIT_REPOSITORY / network access.")
endif()
if(NOT EXISTS "${TETRIS_CUM_ROOT}/src/cum/cum.hpp")
  message(FATAL_ERROR
    "cum headers not found (expected ${TETRIS_CUM_ROOT}/src/cum/cum.hpp). "
    "Set TETRIS_CUM_ROOT or fix TETRIS_CUM_GIT_REPOSITORY / network access.")
endif()

include("${CMAKE_SOURCE_DIR}/cmake/Logless.cmake")
