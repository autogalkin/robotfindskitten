cmake_minimum_required(VERSION 3.24)
project(notepadgame)

set(CMAKE_CXX_STANDARD 20)

set(Boost_INCLUDE_DIR "C:/Program Files/boost/boost_1_81_0")
set(Boost_LIBRARY_DIR "C:/Program Files/boost/boost_1_81_0/libs")
find_package(Boost COMPONENTS system filesystem REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIR})

add_executable(main dllmain.cpp)
target_link_libraries( main ${Boost_LIBRARIES} )


include_directories(loader)
include_directories(notepadgame)
include_directories(notepadgame/core)


add_executable(notepadgame
        loader/main.cpp
        notepadgame/core/common.h
        notepadgame/core/iat_hook.cpp
        notepadgame/core/iat_hook.h
        notepadgame/core/notepad.cpp
        notepadgame/core/notepad.h
        notepadgame/core/outputlog.h
        notepadgame/character.cpp
        notepadgame/character.h
        notepadgame/dllmain.cpp
        notepadgame/input.cpp
        notepadgame/input.h
        notepadgame/world.cpp
        notepadgame/world.h)

