cmake_minimum_required(VERSION 3.16)

# Environment prefix support for Termux cmake module resolution
if(NOT DEFINED ENV{PREFIX} AND EXISTS "/data/data/com.termux/files/usr")
    set(ENV{PREFIX} "/data/data/com.termux/files/usr")
endif()

project(tdecomp C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror -pedantic")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose build type" FORCE)
endif()

# Core decompiled library sources
set(TDECOMP_CORE_SOURCES
    src/nes.c
    src/math/rng.c
    src/math/bcd.c
    src/game/tetrimino.c
    src/game/playfield.c
    src/game/input.c
    src/game/state.c
    src/game/menu.c
    src/game/highscore.c
    src/game/render.c
    src/game/ending.c
    src/game/audio.c
    src/game/system.c
    src/rom_assets.c
)

add_library(tdecomp_core STATIC ${TDECOMP_CORE_SOURCES})
target_include_directories(tdecomp_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Test suite
option(BUILD_TESTING "Build tests" ON)
if(BUILD_TESTING)
    enable_testing()

    set(TDECOMP_TEST_SOURCES
        tests/test_main.c
        tests/test_nes.c
        tests/test_rng.c
        tests/test_bcd.c
        tests/test_tetrimino.c
        tests/test_playfield.c
        tests/test_input.c
        tests/test_state.c
        tests/test_menu.c
        tests/test_highscore.c
        tests/test_render.c
        tests/test_ending.c
        tests/test_audio.c
        tests/test_system.c
        tests/test_rom_assets.c
        native/game_adapter.c
        tests/test_native_adapter.c
        tests/verification/cpu6502.c
        tests/verification/rom_data.c
        tests/verification/test_differential.c
    )

    add_executable(tdecomp_tests ${TDECOMP_TEST_SOURCES})
    target_link_libraries(tdecomp_tests PRIVATE tdecomp_core)
    target_include_directories(tdecomp_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/tests
        ${CMAKE_CURRENT_SOURCE_DIR}/native
    )
    # Ensure assertions remain active in test runner regardless of build type
    target_compile_options(tdecomp_tests PRIVATE -UNDEBUG)

    add_test(NAME tdecomp_tests COMMAND tdecomp_tests)

    add_custom_target(verify
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
        DEPENDS tdecomp_tests
        COMMENT "Run test suite"
    )
endif()

# Native Raylib Desktop Application
option(BUILD_NATIVE_APP "Build native raylib desktop application" ON)
if(BUILD_NATIVE_APP)
    find_package(raylib 4.5 QUIET)
    if(NOT raylib_FOUND)
        find_package(PkgConfig QUIET)
        if(PKG_CONFIG_FOUND)
            pkg_check_modules(RAYLIB QUIET raylib)
        endif()
    endif()

    if(NOT raylib_FOUND AND NOT RAYLIB_FOUND)
        include(FetchContent)
        FetchContent_Declare(
            raylib
            GIT_REPOSITORY https://github.com/raysan5/raylib.git
            GIT_TAG 5.0
        )
        FetchContent_MakeAvailable(raylib)
    endif()

    set(TDECOMP_APP_SOURCES
        native/main.c
        native/game_adapter.c
        native/renderer.c
        native/audio.c
    )

    add_executable(tdecomp ${TDECOMP_APP_SOURCES})
    target_include_directories(tdecomp PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/native
    )
    target_link_libraries(tdecomp PRIVATE tdecomp_core)

    if(TARGET raylib)
        target_link_libraries(tdecomp PRIVATE raylib)
    elseif(raylib_FOUND)
        target_link_libraries(tdecomp PRIVATE raylib)
    elseif(RAYLIB_FOUND)
        target_include_directories(tdecomp PRIVATE ${RAYLIB_INCLUDE_DIRS})
        target_link_libraries(tdecomp PRIVATE ${RAYLIB_LIBRARIES})
    else()
        target_link_libraries(tdecomp PRIVATE raylib)
    endif()

    if(WIN32)
        target_link_libraries(tdecomp PRIVATE winmm gdi32)
    elseif(UNIX AND NOT APPLE)
        target_link_libraries(tdecomp PRIVATE m pthread dl)
    endif()
endif()

# WebAssembly convenience target
find_program(EMCC_EXECUTABLE emcc)
if(EMCC_EXECUTABLE)
    add_custom_target(web
        COMMAND ${CMAKE_MAKE_PROGRAM} -C ${CMAKE_CURRENT_SOURCE_DIR}/web
        COMMENT "Build WebAssembly HTML5 port via Emscripten and raylib"
    )
endif()
