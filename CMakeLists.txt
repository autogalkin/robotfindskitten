cmake_minimum_required(VERSION 3.16)

project(
    robotfindskitten
    VERSION 1.0.0
    LANGUAGES CXX
    )

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


configure_file(${CMAKE_SOURCE_DIR}/src/config.h.in config.h @ONLY)


get_filename_component(
    NPGAME_SCINTILLA_DIR
    "${CMAKE_SOURCE_DIR}/extern/thirdparty/scintilla"
    ABSOLUTE)
# dll injector
add_subdirectory(src/loader)
# game
add_subdirectory(src/robotfindskitten)



find_package(GTest CONFIG REQUIRED)
enable_testing()
add_subdirectory(tests)


add_custom_target(
    copy-compile-commands ALL
    ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_BINARY_DIR}/compile_commands.json
        "${CMAKE_SOURCE_DIR}/compile_commands.json"
    )
