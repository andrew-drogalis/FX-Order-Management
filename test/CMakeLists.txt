cmake_minimum_required(VERSION 3.20)
project(Unit_Test
        VERSION 0.0.1
        DESCRIPTION ""
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(${PROJECT_NAME} unit_test.cpp)

