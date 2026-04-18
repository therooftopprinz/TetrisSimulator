cmake_minimum_required(VERSION 3.16)
project(TetrisSimulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Werror -ggdb3 -O0)

find_package(Threads REQUIRED)

include("${CMAKE_SOURCE_DIR}/cmake/TetrisDeps.cmake")

set(TETRIS_PROTOCOL_CUM "${CMAKE_SOURCE_DIR}/interface/protocol.cum")
set(TETRIS_PROTOCOL_HPP "${CMAKE_SOURCE_DIR}/interface/protocol.hpp")
set(TETRIS_GENERATE_CPP_PY "${CMAKE_SOURCE_DIR}/cum/generate_cpp.py")

find_package(Python3 3 COMPONENTS Interpreter REQUIRED)

add_custom_command(
  OUTPUT "${TETRIS_PROTOCOL_HPP}"
  COMMAND ${CMAKE_COMMAND}
    -D "TETRIS_ROOT=${CMAKE_SOURCE_DIR}"
    -D "PYTHON=${Python3_EXECUTABLE}"
    -P "${CMAKE_SOURCE_DIR}/cmake/GenerateProtocol.cmake"
  DEPENDS "${TETRIS_PROTOCOL_CUM}" "${TETRIS_GENERATE_CPP_PY}"
  COMMENT "Generating interface/protocol.hpp from protocol.cum"
  VERBATIM
)

add_custom_target(tetris_generate_protocol DEPENDS "${TETRIS_PROTOCOL_HPP}")

find_library(TETRIS_NCURSES_LIBRARY NAMES ncursesw ncurses
  PATHS
    /usr/lib/x86_64-linux-gnu
    /lib/x86_64-linux-gnu
    /usr/lib
    /lib
    /usr/lib64
    /lib64
)
if(NOT TETRIS_NCURSES_LIBRARY)
  foreach(cand
      /lib/x86_64-linux-gnu/libncurses.so.6
      /usr/lib/x86_64-linux-gnu/libncurses.so.6
      /lib/x86_64-linux-gnu/libncursesw.so.6
      /usr/lib/x86_64-linux-gnu/libncursesw.so.6
    )
    if(EXISTS "${cand}")
      set(TETRIS_NCURSES_LIBRARY "${cand}")
      break()
    endif()
  endforeach()
endif()
if(NOT TETRIS_NCURSES_LIBRARY)
  message(FATAL_ERROR
    "Could not find libncurses (tried ncursesw/ncurses and common .so.6 paths). "
    "Install libncurses-dev / ncurses-devel, or set TETRIS_NCURSES_LIBRARY to the ncurses shared library.")
endif()

find_library(TETRIS_TINFO_LIBRARY NAMES tinfo
  PATHS
    /usr/lib/x86_64-linux-gnu
    /lib/x86_64-linux-gnu
    /usr/lib
    /lib
    /usr/lib64
    /lib64
)
if(NOT TETRIS_TINFO_LIBRARY)
  foreach(cand
      /lib/x86_64-linux-gnu/libtinfo.so.6
      /usr/lib/x86_64-linux-gnu/libtinfo.so.6
    )
    if(EXISTS "${cand}")
      set(TETRIS_TINFO_LIBRARY "${cand}")
      break()
    endif()
  endforeach()
endif()

set(COMMON_INCLUDE_DIRS
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/common/include
  "${TETRIS_BFC_ROOT}/src"
  "${TETRIS_CUM_ROOT}/src"
  ${CMAKE_SOURCE_DIR}/interface
)

# Bundled GoogleTest / GoogleMock (gmock-gtest-all.cc)
add_library(gtest_all STATIC gtest/gmock-gtest-all.cc)
target_include_directories(gtest_all PUBLIC ${CMAKE_SOURCE_DIR}/gtest)
# Third-party amalgamation; GCC -Werror trips on StackGrowsDown() uninitialized analysis.
target_compile_options(gtest_all PRIVATE -Wno-maybe-uninitialized)

file(GLOB_RECURSE COMMON_LIB_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/common/src/*.cpp")
list(FILTER COMMON_LIB_SOURCES EXCLUDE REGEX ".*/main\\.cpp$")
add_library(common STATIC ${COMMON_LIB_SOURCES})
target_include_directories(common PUBLIC ${COMMON_INCLUDE_DIRS})
target_link_libraries(common PUBLIC logless)
add_dependencies(common tetris_generate_protocol)

file(GLOB_RECURSE SERVER_LIB_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/server/src/*.cpp")
list(FILTER SERVER_LIB_SOURCES EXCLUDE REGEX ".*/main\\.cpp$")
add_library(server_lib STATIC ${SERVER_LIB_SOURCES})
target_include_directories(server_lib PUBLIC ${COMMON_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/server/src)
target_link_libraries(server_lib PUBLIC logless)
add_dependencies(server_lib tetris_generate_protocol)

file(GLOB_RECURSE CLIENT_LIB_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/client/src/*.cpp")
list(FILTER CLIENT_LIB_SOURCES EXCLUDE REGEX ".*/main\\.cpp$")
if(CLIENT_LIB_SOURCES)
  add_library(client_lib STATIC ${CLIENT_LIB_SOURCES})
  target_include_directories(client_lib PUBLIC
    ${COMMON_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/client/src
  )
  target_link_libraries(client_lib PUBLIC ${TETRIS_NCURSES_LIBRARY})
  if(TETRIS_TINFO_LIBRARY)
    target_link_libraries(client_lib PUBLIC ${TETRIS_TINFO_LIBRARY})
  endif()
  target_link_libraries(client_lib PUBLIC logless)
  add_dependencies(client_lib tetris_generate_protocol)
else()
  # client/src may be header-only plus main.cpp (matches empty client.a from configure.py)
  add_library(client_lib INTERFACE)
  target_include_directories(client_lib INTERFACE ${COMMON_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/client/src)
  target_link_libraries(client_lib INTERFACE logless)
endif()

file(GLOB_RECURSE COMMON_TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/common/test/*.cpp")
add_executable(common_test ${COMMON_TEST_SOURCES})
target_include_directories(common_test PRIVATE ${CMAKE_SOURCE_DIR}/gtest ${COMMON_INCLUDE_DIRS})
target_link_libraries(common_test PRIVATE common gtest_all Threads::Threads)

file(GLOB_RECURSE SERVER_TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/server/test/*.cpp")
add_executable(server_test ${SERVER_TEST_SOURCES})
target_include_directories(server_test PRIVATE
  ${CMAKE_SOURCE_DIR}/gtest
  ${COMMON_INCLUDE_DIRS}
  ${CMAKE_SOURCE_DIR}/server/src
  ${CMAKE_SOURCE_DIR}/server/test
)
target_link_libraries(server_test PRIVATE server_lib gtest_all Threads::Threads)

add_executable(server server/src/main.cpp)
target_include_directories(server PRIVATE ${COMMON_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/server/src)
target_link_libraries(server PRIVATE server_lib common Threads::Threads)

file(GLOB_RECURSE CLIENT_TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/client/test/*.cpp")
add_executable(client_test ${CLIENT_TEST_SOURCES})
target_include_directories(client_test PRIVATE
  ${CMAKE_SOURCE_DIR}/gtest
  ${COMMON_INCLUDE_DIRS}
  ${CMAKE_SOURCE_DIR}/client/src
  ${CMAKE_SOURCE_DIR}/client/test
)
target_link_libraries(client_test PRIVATE client_lib common gtest_all Threads::Threads)

add_executable(client client/src/main.cpp)
target_include_directories(client PRIVATE ${COMMON_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/client/src)
target_link_libraries(client PRIVATE client_lib common Threads::Threads)
if(NOT CLIENT_LIB_SOURCES)
  add_dependencies(client tetris_generate_protocol)
  add_dependencies(client_test tetris_generate_protocol)
endif()
