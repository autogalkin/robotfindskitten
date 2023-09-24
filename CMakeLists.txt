cmake_minimum_required(VERSION 3.16)

project(
    notepadgame
    DESCRIPTION "A game inside notepad.exe"
    VERSION 0.1.0
    HOMEPAGE_URL "https://github.com/autogalkin/notepadgame"
    LANGUAGES CXX)

if(NOT
   DEFINED
   ENV{VCPKG_ROOT})
    message(
        FATAL_ERROR
            "You must have vcpkg and set VCPKG_ROOT environment variable")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS
    ON
    CACHE INTERNAL "")
# force msvc to use utf-8
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

get_filename_component(
    NPGAME_SCINTILLA_DIR
    "${CMAKE_SOURCE_DIR}/extern/thirdparty/scintilla"
    ABSOLUTE)
# dll injector
add_subdirectory(src/loader)
# game
add_subdirectory(src/notepadgame)

add_custom_target(
    copy-compile-commands ALL
    ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_BINARY_DIR}/compile_commands.json
        "${CMAKE_SOURCE_DIR}/compile_commands.json"
    )
