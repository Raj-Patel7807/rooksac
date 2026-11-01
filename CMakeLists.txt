cmake_minimum_required(VERSION 3.23)

project(RookSac VERSION 0.1.0 LANGUAGES CXX)

include(CTest)

option(ROOKSAC_ENABLE_CLANG_TIDY "Run clang-tidy while compiling project targets." OFF)
option(ROOKSAC_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer." OFF)
option(ROOKSAC_WARNINGS_AS_ERRORS "Treat project compiler warnings as errors." ON)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

function(rooksac_configure_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
        if(ROOKSAC_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
        if(ROOKSAC_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()

function(rooksac_configure_sanitizers target)
    if(NOT ROOKSAC_ENABLE_SANITIZERS)
        return()
    endif()

    if(MSVC)
        message(FATAL_ERROR "ROOKSAC_ENABLE_SANITIZERS requires a Clang or GCC toolchain.")
    endif()

    target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(${target} PRIVATE -fsanitize=address,undefined)
endfunction()

function(rooksac_configure_clang_tidy target)
    if(NOT ROOKSAC_ENABLE_CLANG_TIDY)
        return()
    endif()

    find_program(
        ROOKSAC_CLANG_TIDY_EXECUTABLE
        NAMES clang-tidy clang-tidy-18 clang-tidy-17 clang-tidy-16 clang-tidy-15
        REQUIRED)
    set_property(
        TARGET ${target}
        PROPERTY CXX_CLANG_TIDY "${ROOKSAC_CLANG_TIDY_EXECUTABLE};--warnings-as-errors=*")
endfunction()

add_executable(rooksac_build_smoke_test engine/tests/build-smoke-test.cpp)
target_compile_features(rooksac_build_smoke_test PRIVATE cxx_std_20)
rooksac_configure_warnings(rooksac_build_smoke_test)
rooksac_configure_sanitizers(rooksac_build_smoke_test)
rooksac_configure_clang_tidy(rooksac_build_smoke_test)

add_library(rooksac_core INTERFACE)
target_compile_features(rooksac_core INTERFACE cxx_std_20)
target_include_directories(rooksac_core INTERFACE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/engine/include>)

add_executable(rooksac_core_types_test engine/tests/core-types-test.cpp)
target_link_libraries(rooksac_core_types_test PRIVATE rooksac_core)
rooksac_configure_warnings(rooksac_core_types_test)
rooksac_configure_sanitizers(rooksac_core_types_test)
rooksac_configure_clang_tidy(rooksac_core_types_test)

if(BUILD_TESTING)
    add_test(NAME rooksac.build-smoke COMMAND rooksac_build_smoke_test)
    add_test(NAME rooksac.core-types COMMAND rooksac_core_types_test)
endif()

add_custom_target(
    format
    COMMAND ${CMAKE_COMMAND} -DROOKSAC_FORMAT_MODE=apply -DROOKSAC_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -P ${CMAKE_SOURCE_DIR}/cmake/format.cmake
    COMMENT "Formatting RookSac C++ sources")

add_custom_target(
    format-check
    COMMAND ${CMAKE_COMMAND} -DROOKSAC_FORMAT_MODE=check -DROOKSAC_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -P ${CMAKE_SOURCE_DIR}/cmake/format.cmake
    COMMENT "Checking RookSac C++ source formatting")
