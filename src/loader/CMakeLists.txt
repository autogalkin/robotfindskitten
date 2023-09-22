
project(loader
        DESCRIPTION ""
        VERSION 0.0.1
        )

add_executable(${PROJECT_NAME} main.cpp)
target_compile_definitions(${PROJECT_NAME} PUBLIC
                                          NOMINMAX            # windows.h
                                          WIN32_LEAN_AND_MEAN # windows.h
                                          _CRT_SECURE_NO_WARNINGS # getenv
                                        )
install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}>
        CONFIGURATIONS Debug
        RUNTIME DESTINATION ./ OPTIONAL)

install(TARGETS ${PROJECT_NAME}
        CONFIGURATIONS Debug
        RUNTIME DESTINATION ./)

install(TARGETS ${PROJECT_NAME}
        CONFIGURATIONS Release
        RUNTIME DESTINATION ./)


