
project(notepadgame
        DESCRIPTION ""
        VERSION 0.0.1
        )

# building scintilla
# find make



find_program(NPGAME_MAKE_EXISTS make mingw32-make)
if(NPGAME_MAKE_EXISTS)
        set(NPGAME_SCIBUILD_COMMAND make)
else()
        find_program(NPGAME_NMAKE_EXISTS  nmake)
        if(NPGAME_NMAKE_EXISTS )
                set(NPGAME_SCIBUILD_COMMAND nmake -f scintilla.mak)
       endif()
endif()
# run the scintilla makefile
add_custom_target(scintilla ALL COMMAND ${NPGAME_SCIBUILD_COMMAND}
                   WORKING_DIRECTORY  "${NPGAME_SCINTILLA_DIR}/win32"
                   COMMENT "Original scintilla makefile target"
)
add_custom_command(
    OUTPUT
        "${CMAKE_CURRENT_BINARY_DIR}/include/foo.h"
    COMMAND ${CMAKE_COMMAND} -E copy
                 "${CMAKE_SOURCE_DIR}/patch/scintilla/src/Editor.cxx" 
                 "${NPGAME_SCINTILLA_DIR}/src/Editor.cxx"
    DEPENDS scintilla
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)
#add_custom_command(
 #       TARGET scintilla POST_BUILD
 #       COMMAND ${CMAKE_COMMAND} -E copy
  #              ${CMAKE_SOURCE_DIR}/test/input.txt
  #              ${CMAKE_CURRENT_BINARY_DIR}/input.txt)


find_package(range-v3 CONFIG REQUIRED)
find_package(Eigen3   CONFIG REQUIRED)
find_package(utf8cpp  CONFIG REQUIRED)
find_package(EnTT     CONFIG REQUIRED)
find_package(Boost COMPONENTS container) # and signals2

file(GLOB_RECURSE SRCS CONFIGURE_DEPENDS *.cpp)
add_library(${PROJECT_NAME} SHARED ${SRCS})

target_link_libraries  (${PROJECT_NAME} PRIVATE  
                                                utf8::cpp
                                                range-v3::range-v3
                                                Eigen3::Eigen
                                                EnTT::EnTT)

target_include_directories(${PROJECT_NAME} PRIVATE "${NPGAME_SCINTILLA_DIR}/include")

target_compile_definitions(${PROJECT_NAME} PUBLIC
        NOMINMAX            # windows.h ...
        WIN32_LEAN_AND_MEAN # windows.h ...
)

install(TARGETS ${PROJECT_NAME}
        CONFIGURATIONS Debug
        RUNTIME DESTINATION /.)

install(TARGETS ${PROJECT_NAME}
        CONFIGURATIONS Release
        RUNTIME DESTINATION ./)

install(FILES "${NPGAME_SCINTILLA_DIR}/bin/Scintilla.dll"
        CONFIGURATIONS Debug
        DESTINATION ./)

install(FILES "${NPGAME_SCINTILLA_DIR}/bin/Scintilla.dll"
        CONFIGURATIONS Release
        DESTINATION ./)