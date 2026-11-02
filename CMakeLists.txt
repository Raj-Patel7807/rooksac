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

add_library(rooksac_core
    engine/src/attacks.cpp
    engine/src/benchmark.cpp
    engine/src/evaluation.cpp
    engine/src/legal_movegen.cpp
    engine/src/match.cpp
    engine/src/material.cpp
    engine/src/move_order.cpp
    engine/src/movegen.cpp
    engine/src/perft.cpp
    engine/src/position.cpp
    engine/src/search.cpp
    engine/src/time_management.cpp
    engine/src/transposition_table.cpp
    engine/src/uci.cpp
    engine/src/zobrist.cpp)
target_compile_features(rooksac_core PUBLIC cxx_std_20)
target_include_directories(rooksac_core PUBLIC $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/engine/include>)
rooksac_configure_warnings(rooksac_core)
rooksac_configure_sanitizers(rooksac_core)
rooksac_configure_clang_tidy(rooksac_core)

add_executable(rooksac_core_types_test engine/tests/core-types-test.cpp)
target_link_libraries(rooksac_core_types_test PRIVATE rooksac_core)
rooksac_configure_warnings(rooksac_core_types_test)
rooksac_configure_sanitizers(rooksac_core_types_test)
rooksac_configure_clang_tidy(rooksac_core_types_test)

add_executable(rooksac_position_test engine/tests/position-test.cpp)
target_link_libraries(rooksac_position_test PRIVATE rooksac_core)
rooksac_configure_warnings(rooksac_position_test)
rooksac_configure_sanitizers(rooksac_position_test)
rooksac_configure_clang_tidy(rooksac_position_test)

add_executable(rooksac_movegen_test engine/tests/movegen-test.cpp)
target_link_libraries(rooksac_movegen_test PRIVATE rooksac_core)
rooksac_configure_warnings(rooksac_movegen_test)
rooksac_configure_sanitizers(rooksac_movegen_test)
rooksac_configure_clang_tidy(rooksac_movegen_test)

add_executable(rooksac_perft_test engine/tests/perft-test.cpp)
target_link_libraries(rooksac_perft_test PRIVATE rooksac_core)
rooksac_configure_warnings(rooksac_perft_test)
rooksac_configure_sanitizers(rooksac_perft_test)
rooksac_configure_clang_tidy(rooksac_perft_test)

add_executable(rooksac_agent_test engine/tests/agent-test.cpp)
target_link_libraries(rooksac_agent_test PRIVATE rooksac_core)
rooksac_configure_warnings(rooksac_agent_test)
rooksac_configure_sanitizers(rooksac_agent_test)
rooksac_configure_clang_tidy(rooksac_agent_test)

add_executable(rooksac_search_test engine/tests/search-test.cpp)
target_link_libraries(rooksac_search_test PRIVATE rooksac_core)
rooksac_configure_warnings(rooksac_search_test)
rooksac_configure_sanitizers(rooksac_search_test)
rooksac_configure_clang_tidy(rooksac_search_test)

add_executable(rooksac_match_harness engine/src/match_harness.cpp)
target_link_libraries(rooksac_match_harness PRIVATE rooksac_core)
rooksac_configure_warnings(rooksac_match_harness)
rooksac_configure_sanitizers(rooksac_match_harness)
rooksac_configure_clang_tidy(rooksac_match_harness)

add_executable(rooksac_uci_test engine/tests/uci-test.cpp)
target_link_libraries(rooksac_uci_test PRIVATE rooksac_core)
rooksac_configure_warnings(rooksac_uci_test)
rooksac_configure_sanitizers(rooksac_uci_test)
rooksac_configure_clang_tidy(rooksac_uci_test)

add_executable(rooksac_zobrist_tt_test engine/tests/zobrist-tt-test.cpp)
target_link_libraries(rooksac_zobrist_tt_test PRIVATE rooksac_core)
rooksac_configure_warnings(rooksac_zobrist_tt_test)
rooksac_configure_sanitizers(rooksac_zobrist_tt_test)
rooksac_configure_clang_tidy(rooksac_zobrist_tt_test)

add_executable(rooksac_eval_test engine/tests/eval-test.cpp)
target_link_libraries(rooksac_eval_test PRIVATE rooksac_core)
rooksac_configure_warnings(rooksac_eval_test)
rooksac_configure_sanitizers(rooksac_eval_test)
rooksac_configure_clang_tidy(rooksac_eval_test)

add_executable(rooksac engine/apps/rooksac_main.cpp)
target_link_libraries(rooksac PRIVATE rooksac_core)
rooksac_configure_warnings(rooksac)
rooksac_configure_sanitizers(rooksac)
rooksac_configure_clang_tidy(rooksac)

add_executable(rooksac_bench engine/apps/rooksac_bench_main.cpp)
target_link_libraries(rooksac_bench PRIVATE rooksac_core)
rooksac_configure_warnings(rooksac_bench)
rooksac_configure_sanitizers(rooksac_bench)
rooksac_configure_clang_tidy(rooksac_bench)

if(BUILD_TESTING)
    add_test(NAME rooksac.build-smoke COMMAND rooksac_build_smoke_test)
    add_test(NAME rooksac.core-types COMMAND rooksac_core_types_test)
    add_test(NAME rooksac.position COMMAND rooksac_position_test)
    add_test(NAME rooksac.movegen COMMAND rooksac_movegen_test)
    add_test(NAME rooksac.perft COMMAND rooksac_perft_test)
    add_test(NAME rooksac.agent COMMAND rooksac_agent_test)
    add_test(NAME rooksac.search COMMAND rooksac_search_test)
    add_test(NAME rooksac.uci COMMAND rooksac_uci_test)
    add_test(NAME rooksac.zobrist_tt COMMAND rooksac_zobrist_tt_test)
    add_test(NAME rooksac.eval COMMAND rooksac_eval_test)
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
