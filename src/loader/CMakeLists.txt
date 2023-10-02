project(
    loader LANGUAGES CXX)

add_executable(${PROJECT_NAME} main.cpp)
target_compile_definitions(
    ${PROJECT_NAME}
    PUBLIC # windows.h
           NOMINMAX
           WIN32_LEAN_AND_MEAN
           _CRT_SECURE_NO_WARNINGS # getenv
)
target_include_directories(${PROJECT_NAME} PUBLIC
                           "${CMAKE_BINARY_DIR}"
                           )
install(
    FILES $<TARGET_PDB_FILE:${PROJECT_NAME}>
    CONFIGURATIONS Debug RUNTIME
    DESTINATION ./
    OPTIONAL)

install(
    TARGETS ${PROJECT_NAME}
    CONFIGURATIONS Debug
    RUNTIME DESTINATION ./)

install(
    TARGETS ${PROJECT_NAME}
    CONFIGURATIONS Release
    RUNTIME DESTINATION ./)
