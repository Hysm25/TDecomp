cmake_minimum_required(VERSION 3.16)

# Termux / Android environment support
if(NOT DEFINED ENV{PREFIX} AND EXISTS "/data/data/com.termux/files/usr")
    set(ENV{PREFIX} "/data/data/com.termux/files/usr")
endif()

project(tdecomp C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror -pedantic -Wno-unused-parameter")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose build type" FORCE)
endif()

# Core include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Core decompiled library sources
set(TDECOMP_CORE_SOURCES
    src/nes.c
    src/math/rng.c
    src/math/bcd.c
    src/game/tetrimino.c
    src/game/playfield.c
)

add_library(tdecomp_core STATIC ${TDECOMP_CORE_SOURCES})

# Test suite
option(BUILD_TESTING "Build tests" ON)
if(BUILD_TESTING)
    enable_testing()

    include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )

    set(TDECOMP_TEST_SOURCES
        tests/test_main.c
        tests/test_nes.c
        tests/test_rng.c
        tests/test_bcd.c
        tests/test_tetrimino.c
        tests/test_playfield.c
        tests/verification/cpu6502.c
        tests/verification/rom_data.c
        tests/verification/test_differential.c
    )

    add_executable(tdecomp_tests ${TDECOMP_TEST_SOURCES})
    target_link_libraries(tdecomp_tests PRIVATE tdecomp_core)

    add_test(NAME tdecomp_tests COMMAND tdecomp_tests)
endif()
