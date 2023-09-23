project(
    notepadgamedll
    DESCRIPTION ""
    VERSION 0.0.1)

# patch scintilla source code
set(NPGAME_SCI_PATCH_FILE
    "${CMAKE_SOURCE_DIR}/extern/patch/scintilla/src/Editor.cxx")
set(NPGAME_SCI_ORIG_FILE "${NPGAME_SCINTILLA_DIR}/src/Editor.cxx")
set(NPGAME_SCI_TEMP_FILE "${CMAKE_CURRENT_BINARY_DIR}/Editor_temp.cxx")
set(NPGAME_SCI_OUT_FILE "${NPGAME_SCINTILLA_DIR}/bin/Scintilla.dll")

set(SCI_PACHED_STAMP
    "${CMAKE_CURRENT_BINARY_DIR}/scintilla_already_patched.stamp")
add_custom_command(
    OUTPUT ${SCI_PACHED_STAMP}
    COMMAND ${CMAKE_COMMAND} -E copy "${NPGAME_SCI_ORIG_FILE}"
            "${NPGAME_SCI_TEMP_FILE}"
    COMMAND ${CMAKE_COMMAND} -E copy "${NPGAME_SCI_PATCH_FILE}"
            "${NPGAME_SCI_ORIG_FILE}"
    COMMAND ${CMAKE_COMMAND} -E touch "${SCI_PACHED_STAMP}"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    DEPENDS "${NPGAME_SCI_PATCH_FILE}"
    COMMENT "Replace Scintilla source code files with patched own")

# find make
find_program(NPGAME_NMAKE_EXISTS nmake)
if(NPGAME_NMAKE_EXISTS)
    set(NPGAME_SCIBUILD_COMMAND nmake -f scintilla.mak)
else()
    message("Nmake not found, try to find Make and GCC")
    find_program(
        NPGAME_MAKE_EXISTS
        make
        mingw32-make)
    find_program(
        NPGAME_GCC_EXISTS
        gcc
        g++)
    if(NPGAME_MAKE_EXISTS AND NPGAME_GCC_EXISTS)
        set(NPGAME_SCIBUILD_COMMAND make)
    else()
        if(NOT NPGAME_MAKE_EXISTS)
            message(SEND_ERROR "Make not found")
        endif()
        if(NOT NPGAME_GCC_EXISTS)
            message(SEND_ERROR "GCC not found")
        endif()
        message(FATAL_ERROR "Unable to compile Scintilla..")

    endif()
endif()

# run scintilla makefile
add_custom_target(
    scintilla ALL
    COMMAND ${NPGAME_SCIBUILD_COMMAND}
    BYPRODUCT ${NPGAME_SCI_OUT_FILE}
    WORKING_DIRECTORY "${NPGAME_SCINTILLA_DIR}/win32"
    DEPENDS ${SCI_PACHED_STAMP}
    COMMENT "Build Scintilla with original Makefile")

# revert patch after build Scintilla
add_custom_target(
    revert_scintilla_patch
    COMMAND ${CMAKE_COMMAND} -E copy "${NPGAME_SCI_TEMP_FILE}"
            "${NPGAME_SCI_ORIG_FILE}"
    COMMAND ${CMAKE_COMMAND} -E remove "${NPGAME_SCI_TEMP_FILE}"
    COMMENT "Revert Scintilla patches")
add_dependencies(revert_scintilla_patch scintilla)

# build notepadgame.dll

find_package(
    range-v3
    CONFIG
    REQUIRED)
find_package(
    Eigen3
    CONFIG
    REQUIRED)
find_package(
    EnTT
    CONFIG
    REQUIRED)
find_package(Boost COMPONENTS container) # and header-only signals2

# include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# project splits into static libraries
include(engine/CMakeLists.txt)
include(game/CMakeLists.txt)

add_library(engine STATIC ${ENGINE_SOURCES} ${ENGINE_HEADERS})
add_library(game STATIC ${GAME_SOURCES} ${GAME_HEADERS})
add_library(${PROJECT_NAME} SHARED ./dllmain.cpp)

set(THIRD_PARTY_LIBRARIES
    ${Boost_LIBRARIES}
    range-v3::range-v3
    Eigen3::Eigen
    EnTT::EnTT)

add_subdirectory(libs/easing)

list(
    APPEND
    NOTEPADGAME_ALL_TARGETS
    engine
    game
    ${PROJECT_NAME})
foreach(_target IN LISTS NOTEPADGAME_ALL_TARGETS)
    set_target_properties(
        ${_target}
        PROPERTIES CXX_STANDARD 20
                   CXX_STANDARD_REQUIRED ON
                   LINKER_LANGUAGE CXX)
    target_compile_definitions(${_target} PUBLIC # windows.h
                                                 NOMINMAX WIN32_LEAN_AND_MEAN)
    target_include_directories(
        ${_target}
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
                ${Boost_INCLUDE_DIRS}
                "${CMAKE_CURRENT_SOURCE_DIR}/libs"
                "${CMAKE_SOURCE_DIR}/extern/thirdparty/dirtyflag/include/"
                "${NPGAME_SCINTILLA_DIR}/include/")
endforeach()

target_link_libraries(engine PRIVATE ${THIRD_PARTY_LIBRARIES})
target_link_libraries(game PRIVATE engine ${THIRD_PARTY_LIBRARIES})
target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE engine
            game
            ${THIRD_PARTY_LIBRARIES})


install(
    TARGETS ${PROJECT_NAME}
    RUNTIME_DEPENDENCIES $<TARGET_PDB_FILE:${PROJECT_NAME}> 
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

install(
    FILES "${NPGAME_SCI_OUT_FILE}"
    CONFIGURATIONS Debug
    DESTINATION .)

install(
    FILES "${NPGAME_SCI_OUT_FILE}"
    CONFIGURATIONS Release
    DESTINATION .)
