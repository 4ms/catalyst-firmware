cmake_minimum_required(VERSION 3.15)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_LIST_DIR}/cmake/arm-none-eabi-gcc.cmake)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_BUILD_TYPE "RelWithDebInfo")

project(
  MacroSeq
  VERSION 1.0
  DESCRIPTION "MacroSeq"
  LANGUAGES C CXX ASM
)

include(cmake/project.cmake)

add_subdirectory(src/f401-drivers f401)

add_custom_target(
  tests ALL
  COMMAND cd ${PROJECT_SOURCE_DIR} && make -f tests/Makefile
  USES_TERMINAL
)

add_custom_target(
  dep_tests
  COMMAND cd ${PROJECT_SOURCE_DIR}/lib/cpputil && make --no-print-directory -f tests/Makefile
  COMMAND cd ${PROJECT_SOURCE_DIR}/lib/mdrivlib && make --no-print-directory -f tests/Makefile
  USES_TERMINAL
)
