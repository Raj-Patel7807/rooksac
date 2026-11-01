if(NOT DEFINED ROOKSAC_FORMAT_MODE OR NOT DEFINED ROOKSAC_SOURCE_DIR)
    message(FATAL_ERROR "ROOKSAC_FORMAT_MODE and ROOKSAC_SOURCE_DIR are required.")
endif()

if(NOT ROOKSAC_FORMAT_MODE STREQUAL "apply" AND NOT ROOKSAC_FORMAT_MODE STREQUAL "check")
    message(FATAL_ERROR "ROOKSAC_FORMAT_MODE must be either 'apply' or 'check'.")
endif()

find_program(
    ROOKSAC_CLANG_FORMAT_EXECUTABLE
    NAMES clang-format clang-format-18 clang-format-17 clang-format-16 clang-format-15
    REQUIRED)

file(
    GLOB_RECURSE ROOKSAC_FORMAT_SOURCES
    LIST_DIRECTORIES FALSE
    "${ROOKSAC_SOURCE_DIR}/engine/*.c"
    "${ROOKSAC_SOURCE_DIR}/engine/*.cc"
    "${ROOKSAC_SOURCE_DIR}/engine/*.cpp"
    "${ROOKSAC_SOURCE_DIR}/engine/*.cxx"
    "${ROOKSAC_SOURCE_DIR}/engine/*.h"
    "${ROOKSAC_SOURCE_DIR}/engine/*.hh"
    "${ROOKSAC_SOURCE_DIR}/engine/*.hpp"
    "${ROOKSAC_SOURCE_DIR}/engine/*.hxx")
list(SORT ROOKSAC_FORMAT_SOURCES)

if(NOT ROOKSAC_FORMAT_SOURCES)
    message(FATAL_ERROR "No C++ source files were found under engine/.")
endif()

foreach(source_file IN LISTS ROOKSAC_FORMAT_SOURCES)
    if(ROOKSAC_FORMAT_MODE STREQUAL "apply")
        execute_process(
            COMMAND "${ROOKSAC_CLANG_FORMAT_EXECUTABLE}" -i "${source_file}"
            COMMAND_ERROR_IS_FATAL ANY)
    else()
        execute_process(
            COMMAND "${ROOKSAC_CLANG_FORMAT_EXECUTABLE}" --dry-run --Werror "${source_file}"
            COMMAND_ERROR_IS_FATAL ANY)
    endif()
endforeach()
