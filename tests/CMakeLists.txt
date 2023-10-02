project(test)

set(TEST_SOURCES   collision_test.cpp)


add_executable(test
    ${TEST_SOURCES}
)

set_target_properties(
        ${PROJECT_NAME}
        PROPERTIES CXX_STANDARD 20
                   CXX_STANDARD_REQUIRED ON
                   LINKER_LANGUAGE CXX)
target_compile_definitions( ${PROJECT_NAME} PUBLIC # windows.h
                                NOMINMAX WIN32_LEAN_AND_MEAN)

target_link_libraries( ${PROJECT_NAME} PUBLIC
        robotfindskitten
        GTest::gtest_main)

set(PROJECT_FOR_TEST_DIR "${CMAKE_SOURCE_DIR}/src/robotfindskitten/")
target_include_directories(${PROJECT_NAME} PRIVATE "${GTEST_INCLUDE_DIR}" "${PROJECT_FOR_TEST_DIR}")
target_link_libraries(${PROJECT_NAME} robotfindskitten "${GTEST_LIBRARIES}" "${GTEST_MAIN_LIBRARIES}" pthread)
gtest_discover_tests(${PROJECT_NAME}
        # set a working directory so your project root so that you can find test data via paths relative to the project root
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )
    
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER tests)
#include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
#target_link_libraries(mytest gtest gtest_main)
#target_include_directories(${GTEST_INCLUDE_DIR})
# Link test executable against gtest & gtest_main
#target_link_libraries(test ${GTEST_LIBRARY_DEBUG} ${GTEST_MAIN_LIBRARY_DEBUG})
#target_link_libraries(${PROJECT_NAME} PRIVATE GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main)

#target_link_libraries(
# test
# PRIVATE GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main
#)
#
#include_directories(${GTEST_INCLUDE_DIR})
#
#add_custom_target( check COMMAND ${CMAKE_CTEST_COMMAND} --verbose )
#include(GoogleTest)
#gtest_discover_tests(test)
