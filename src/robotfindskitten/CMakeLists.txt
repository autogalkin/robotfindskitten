project(
    robotfindskitten LANGUAGES CXX
    )

include(FetchContent)
FetchContent_Declare(
    scintilla
    URL "https://sourceforge.net/projects/scintilla/files/scintilla/5.3.7/scintilla537.zip/download"
    URL_HASH "SHA1=7a72a85442706acbcb4075816a5cc4e58a6bc21c"
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SOURCE_DIR ${CMAKE_BINARY_DIR}/_deps/scintilla
)
FetchContent_Declare(
    lexilla
    URL "https://sourceforge.net/projects/scintilla/files/lexilla/5.2.7/lexilla527.zip/download"
    URL_HASH "SHA1=f46943e10cad2677d503970b671710341e2db42f"
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SOURCE_DIR ${CMAKE_BINARY_DIR}/_deps/lexilla
)
FetchContent_MakeAvailable(lexilla scintilla)


# patch scintilla source code
set(SCI_PATCH_FILE
"${CMAKE_SOURCE_DIR}/extern/scintilla-patch/src/Editor.cxx")
set(SCI_ORIG_FILE "${scintilla_SOURCE_DIR}/src/Editor.cxx")
set(SCI_TEMP_FILE "${CMAKE_CURRENT_BINARY_DIR}/Editor_temp.cxx")
set(SCI_OUT_FILE "${scintilla_SOURCE_DIR}/bin/Scintilla.dll")
set(LEX_OUT_FILE "${lexilla_SOURCE_DIR}/bin/Lexilla.dll")
set(SCI_PACHED_STAMP
"${CMAKE_CURRENT_BINARY_DIR}/scintilla_already_patched.stamp")


add_custom_command(
        OUTPUT ${SCI_PACHED_STAMP}
        COMMAND ${CMAKE_COMMAND} -E copy "${SCI_ORIG_FILE}"
                "${SCI_TEMP_FILE}"
        COMMAND ${CMAKE_COMMAND} -E copy "${SCI_PATCH_FILE}"
                "${SCI_ORIG_FILE}"
        COMMAND ${CMAKE_COMMAND} -E touch "${SCI_PACHED_STAMP}"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        DEPENDS "${SCI_PATCH_FILE}"
        COMMENT "Replace Scintilla source code files with patched files"
    )

# find make
find_program(IS_NMAKE_EXISTS nmake)
if(IS_NMAKE_EXISTS)
    set(SCI_BUILD_COMMAND nmake -f scintilla.mak)
else()
    message("Nmake not found, try to find Make and GCC")
    find_program(
        IS_MAKE_EXISTS
        make
        mingw32-make)
    find_program(
        IS_GCC_EXISTS
        gcc
        g++)
    if(IS_MAKE_EXISTS AND IS_GCC_EXISTS)
        set(SCI_BUILD_COMMAND make)
    else()
        if(NOT IS_MAKE_EXISTS)
            message(SEND_ERROR "Make not found")
        endif()
        if(NOT IS_GCC_EXISTS)
            message(SEND_ERROR "GCC not found")
        endif()
        message(FATAL_ERROR "Unable to compile Scintilla..")

    endif()
endif()

# Run scintilla makefile
add_custom_target(
        scintilla
        COMMAND ${SCI_BUILD_COMMAND}
        WORKING_DIRECTORY "${scintilla_SOURCE_DIR}/win32"
        DEPENDS "${SCI_PACHED_STAMP}"
        COMMENT "Build Scintilla with original Makefile")
if(NOT EXISTS "${SCI_OUT_FILE}")
    set_target_properties(scintilla PROPERTIES EXCLUDE_FROM_ALL OFF)
else()
    set_target_properties(scintilla PROPERTIES EXCLUDE_FROM_ALL ON)
endif()

# Revert the patch after building Scintilla
add_custom_target(
        revert_scintilla_patch
        COMMAND ${CMAKE_COMMAND} -E copy "${NPGAME_SCI_TEMP_FILE}"
                "${SCI_ORIG_FILE}"
        COMMAND ${CMAKE_COMMAND} -E remove "${NPGAME_SCI_TEMP_FILE}"
        COMMENT "Revert the patch from Scintilla"
    )
add_dependencies(revert_scintilla_patch scintilla)

install(
        FILES "${SCI_OUT_FILE}"
        CONFIGURATIONS Debug
        DESTINATION . )

install(
        FILES "${SCI_OUT_FILE}"
        CONFIGURATIONS Release
        DESTINATION . )

#endif()

# build robotfindskitten.dll


find_package(
    glm
    CONFIG
    REQUIRED)
find_package(
    EnTT
    CONFIG
    REQUIRED)
find_package(Boost COMPONENTS container) # and header-only signals2


# project splits into static libraries !IMPORTANT include needs to make include
# namespaces for libraries e.g notepad.h -> engine/notepad.h
include(engine/CMakeLists.txt)
include(game/CMakeLists.txt)

add_library(engine STATIC ${ENGINE_SOURCES} ${ENGINE_HEADERS})
add_library(game STATIC ${GAME_SOURCES} ${GAME_HEADERS})

file(READ "${CMAKE_CURRENT_LIST_DIR}/game/assets/messages.txt" MESSAGES_FILE_CONTENT)
string(REPLACE "\n" ";" MESSAGES_LIST ${MESSAGES_FILE_CONTENT})
list(FILTER MESSAGES_LIST EXCLUDE REGEX "^$" )
list(TRANSFORM MESSAGES_LIST STRIP )
list(TRANSFORM MESSAGES_LIST REPLACE "\"" "" )
list(TRANSFORM MESSAGES_LIST APPEND "\"")
list(TRANSFORM MESSAGES_LIST PREPEND "\"")
string(REPLACE ";" "," ALL_GAME_MESSAGES "{${MESSAGES_LIST}}")
configure_file(${CMAKE_CURRENT_LIST_DIR}/game/messages.h.in "${CMAKE_CURRENT_BINARY_DIR}/messages.h" @ONLY)
target_include_directories(game PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")


add_library(${PROJECT_NAME} SHARED ./dllmain.cpp)

file(GLOB LEX_SOURCES CONFIGURE_DEPENDS ${lexilla_SOURCE_DIR}/lexlib/*.cxx)
file(GLOB LEX_HEADERS CONFIGURE_DEPENDS ${lexilla_SOURCE_DIR}/lexlib/*.h)
add_library(lexlib STATIC ${LEX_SOURCES} ${LEX_HEADERS})

set_target_properties(
        lexlib
        PROPERTIES CXX_STANDARD 20
                   CXX_STANDARD_REQUIRED ON
                   LINKER_LANGUAGE CXX)
target_compile_definitions(lexlib PUBLIC # windows.h
                                NOMINMAX WIN32_LEAN_AND_MEAN)
target_include_directories(lexlib PRIVATE
"${scintilla_SOURCE_DIR}/include"
"${lexilla_SOURCE_DIR}/lexlib"
"${lexilla_SOURCE_DIR}/include"
)


set(THIRD_PARTY_LIBRARIES
    ${Boost_LIBRARIES}
    glm::glm
    EnTT::EnTT
    )

list(
    APPEND
    ROBOTFINDSKITTEN_ALL_TARGETS
    engine
    game
    ${PROJECT_NAME})
foreach(_target IN LISTS ROBOTFINDSKITTEN_ALL_TARGETS)
    set_target_properties(
        ${_target}
        PROPERTIES CXX_STANDARD 20
                   CXX_STANDARD_REQUIRED ON
                   LINKER_LANGUAGE CXX)
    target_compile_definitions(${_target} PRIVATE # windows.h
                            NOMINMAX WIN32_LEAN_AND_MEAN)
    target_include_directories(
        ${_target}
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
                ${Boost_INCLUDE_DIRS}
                "${scintilla_SOURCE_DIR}/include"
                "${lexilla_SOURCE_DIR}/lexlib"
                "${lexilla_SOURCE_DIR}/include"
                "${CMAKE_BINARY_DIR}"
                )
endforeach()

target_link_libraries(engine PRIVATE ${THIRD_PARTY_LIBRARIES})
target_link_libraries(game PRIVATE engine ${THIRD_PARTY_LIBRARIES} lexlib)
target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE engine
            game
            lexlib
            ${THIRD_PARTY_LIBRARIES})

install(
    TARGETS ${PROJECT_NAME}
    CONFIGURATIONS Debug
    RUNTIME DESTINATION .)
install(
    FILES $<TARGET_PDB_FILE:${PROJECT_NAME}>
    CONFIGURATIONS Debug RUNTIME
    DESTINATION .
    OPTIONAL)

install(
    TARGETS ${PROJECT_NAME}
    CONFIGURATIONS Release
    RUNTIME DESTINATION .)
