# Invoked by add_custom_command via: cmake -P cmake/GenerateProtocol.cmake
# Required: -D TETRIS_ROOT=... -D PYTHON=...
if(NOT DEFINED TETRIS_ROOT OR NOT DEFINED PYTHON)
  message(FATAL_ERROR "GenerateProtocol.cmake requires TETRIS_ROOT and PYTHON")
endif()
execute_process(
  COMMAND "${PYTHON}" "${TETRIS_ROOT}/cum/generate_cpp.py" "${TETRIS_ROOT}/interface/protocol.cum"
  OUTPUT_FILE "${TETRIS_ROOT}/interface/protocol.hpp"
  RESULT_VARIABLE _rc
  ERROR_VARIABLE _err
)
if(NOT _rc EQUAL 0)
  message(FATAL_ERROR "cum/generate_cpp.py failed (${_rc}): ${_err}")
endif()
