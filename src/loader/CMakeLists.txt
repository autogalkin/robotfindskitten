
project(loader
        DESCRIPTION ""
        VERSION 0.0.1
        )

add_executable(${PROJECT_NAME} main.cpp)

install(TARGETS ${PROJECT_NAME}
        CONFIGURATIONS Debug
        RUNTIME DESTINATION ./)

install(TARGETS ${PROJECT_NAME}
        CONFIGURATIONS Release
        RUNTIME DESTINATION ./)


