#
# loader
#

project(loader
        DESCRIPTION ""
        VERSION 0.0.1
        )


file (GLOB SOURCES "src/*.cpp")

add_executable(${PROJECT_NAME} main.cpp)

install(TARGETS ${PROJECT_NAME}
        CONFIGURATIONS Debug
        RUNTIME DESTINATION Debug/)

install(TARGETS ${PROJECT_NAME}
        CONFIGURATIONS Release
        RUNTIME DESTINATION Release/)


