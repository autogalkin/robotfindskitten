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


option(ROBOTFINDSKITTEN_BUILD_TESTING "Enable building tests." OFF)

if(ROBOTFINDSKITTEN_BUILD_TESTING)
    find_package(GTest CONFIG REQUIRED)
    enable_testing()
    add_subdirectory(tests)
endif()

option(ROBOTFINDSKITTEN_BUILD_DOCS "Enable building with documentation." OFF)

if(ROBOTFINDSKITTEN_BUILD_DOCS)
    find_package(Doxygen)
    configure_file(DoxygenFile.in DoxygenFile @ONLY)
    add_custom_target(
    docs ALL
    COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/DoxygenFile
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/src/"
    VERBATIM
    SOURCES
        "${CMAKE_CURRENT_BINARY_DIR}/DoxygenFile"
    )
    install(
        DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html
        DESTINATION docs
    )
endif()

add_custom_target(
    copy-compile-commands ALL
    ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_BINARY_DIR}/compile_commands.json
        "${CMAKE_SOURCE_DIR}/compile_commands.json"
    )
