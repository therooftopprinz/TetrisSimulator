cmake_minimum_required(VERSION 3.16)
project(TetrisSimulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Werror -ggdb3 -O0)

find_package(Threads REQUIRED)

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
  ${CMAKE_SOURCE_DIR}/Logless/src
  ${CMAKE_SOURCE_DIR}/common/include
  ${CMAKE_SOURCE_DIR}/BFC/src
  ${CMAKE_SOURCE_DIR}/cum/src
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

file(GLOB_RECURSE SERVER_LIB_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/server/src/*.cpp")
list(FILTER SERVER_LIB_SOURCES EXCLUDE REGEX ".*/main\\.cpp$")
add_library(server_lib STATIC ${SERVER_LIB_SOURCES})
target_include_directories(server_lib PUBLIC ${COMMON_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/server/src)

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
else()
  # client/src may be header-only plus main.cpp (matches empty client.a from configure.py)
  add_library(client_lib INTERFACE)
  target_include_directories(client_lib INTERFACE ${COMMON_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/client/src)
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/Logless/build/liblogless.a")
  set(LOGLESS_A "${CMAKE_SOURCE_DIR}/Logless/build/liblogless.a")
elseif(EXISTS "${CMAKE_SOURCE_DIR}/Logless/build/logless.a")
  set(LOGLESS_A "${CMAKE_SOURCE_DIR}/Logless/build/logless.a")
else()
  message(FATAL_ERROR
    "Missing Logless static library under Logless/build/ (expected liblogless.a or logless.a).\n"
    "From the repo root, initialize submodules and build Logless (e.g. cmake -S Logless -B Logless/build && cmake --build Logless/build).")
endif()
add_library(logless STATIC IMPORTED)
set_target_properties(logless PROPERTIES IMPORTED_LOCATION "${LOGLESS_A}")

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
target_link_libraries(server_test PRIVATE server_lib gtest_all logless Threads::Threads)

add_executable(server server/src/main.cpp)
target_include_directories(server PRIVATE ${COMMON_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/server/src)
target_link_libraries(server PRIVATE server_lib common logless Threads::Threads)

file(GLOB_RECURSE CLIENT_TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/client/test/*.cpp")
add_executable(client_test ${CLIENT_TEST_SOURCES})
target_include_directories(client_test PRIVATE
  ${CMAKE_SOURCE_DIR}/gtest
  ${COMMON_INCLUDE_DIRS}
  ${CMAKE_SOURCE_DIR}/client/src
  ${CMAKE_SOURCE_DIR}/client/test
)
target_link_libraries(client_test PRIVATE client_lib common gtest_all logless Threads::Threads)

add_executable(client client/src/main.cpp)
target_include_directories(client PRIVATE ${COMMON_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/client/src)
target_link_libraries(client PRIVATE client_lib common logless Threads::Threads)
