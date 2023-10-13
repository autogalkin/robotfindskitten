project(robotfindskitten_tests LANGUAGES CXX)

set(TEST_SOURCES   test_collision.cpp)

include(GoogleTest)

foreach(source_file ${TEST_SOURCES})
    get_filename_component(target_name ${source_file} NAME_WE)
    add_executable(${target_name}  ${source_file})
    set_target_properties(${target_name}
        PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tests"
        LINKER_LANGUAGE CXX
        )
    target_compile_definitions( ${target_name} PUBLIC # windows.h
                                NOMINMAX WIN32_LEAN_AND_MEAN)

    target_include_directories(${target_name}
        PRIVATE
            "${GTEST_INCLUDE_DIR}"
            "${CMAKE_SOURCE_DIR}/src/robotfindskitten/"
    )
    target_link_libraries(${target_name}
        PRIVATE
        GTest::gmock_main
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        "${GTEST_LIBRARIES}"
        "${GTEST_MAIN_LIBRARIES}"
        game
        engine
    )
    gtest_discover_tests(${target_name}
# set a working directory so your project root so that you can find test data via paths relative to the project root
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
    set_target_properties(${target_name} PROPERTIES FOLDER tests)
    # set_target_properties(${target_name} PROPERTIES EXCLUDE_FROM_ALL ON)
    list(APPEND TEST_TARGETS ${target_name})
endforeach()
