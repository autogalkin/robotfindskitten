
cmake_minimum_required(VERSION 3.16)


project(notepadgame
        DESCRIPTION "A hello world project"
        VERSION 0.1.0
        HOMEPAGE_URL "https://github.com/autogalkin/notepadgame"
        LANGUAGES CXX
        )

set(CMAKE_CXX_STANDARD          20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "") 
# force msvc to use utf-8
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

get_filename_component(NPGAME_SCINTILLA_DIR
                       "${CMAKE_SOURCE_DIR}/extern/thirdparty/scintilla"
                       ABSOLUTE)
# compile the dll injector
add_subdirectory(src/loader)
# compile engine
add_subdirectory(src/notepadgame)

#create a hard link for the compile_commands.json in the root directory for clangd
execute_process(
    COMMAND ${CMAKE_COMMAND} -E create_hardlink "${CMAKE_BINARY_DIR}/compile_commands.json" "${CMAKE_SOURCE_DIR}/compile_commands.json"
     COMMAND_ECHO STDOUT
    )
