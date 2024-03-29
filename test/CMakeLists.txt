cmake_minimum_required(VERSION 3.20)
project(Unit_Test
        VERSION 0.0.1
        DESCRIPTION ""
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(${PROJECT_NAME} unit_test.cpp)

find_package(GTest)

# enable CTest testing
enable_testing()

cmake_path(GET CMAKE_CURRENT_SOURCE_DIR PARENT_PATH PARENT_DIR)

target_include_directories(${PROJECT_NAME} PRIVATE ${PARENT_DIR}/include)

target_link_libraries(${PROJECT_NAME}
    GTest::GTest 
    GTest::Main
)

add_test(test_all unit_tests)