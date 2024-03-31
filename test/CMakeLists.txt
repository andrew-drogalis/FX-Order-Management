cmake_minimum_required(VERSION 3.20)
project(Unit_Test
        VERSION 0.0.1
        DESCRIPTION ""
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(GNUInstallDirs)

cmake_path(GET CMAKE_CURRENT_SOURCE_DIR PARENT_PATH PARENT_DIR)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CPR_INCLUDE_DIRS}
    ${GTEST_INCLUDE_DIRS}
)

find_package(GTest)

enable_testing()

add_executable(${PROJECT_NAME} 
    unit_test.cpp
    ${PARENT_DIR}/src/fx_order_management.cpp
    ${PARENT_DIR}/src/trading_model.cpp
    ${PARENT_DIR}/src/fx_market_time.cpp
    ${PARENT_DIR}/src/fx_utilities.cpp)

include(${PARENT_DIR}/cmake/Tests.cmake)
include(${PARENT_DIR}/cmake/Hardening.cmake)
include(${PARENT_DIR}/cmake/PreventInSourceBuilds.cmake)
include(${PARENT_DIR}/cmake/ClangFormat.cmake)
include(${PARENT_DIR}/cmake/Keychain.cmake)

build_keychain(${PROJECT_NAME} ${PARENT_DIR})

target_include_directories(${PROJECT_NAME} PRIVATE ${PARENT_DIR}/include)

target_link_libraries(${PROJECT_NAME} PRIVATE cpr::cpr)

include_directories( ${Boost_INCLUDE_DIRS} ${PARENT_DIR}/lib)

add_library( gain_capital_ SHARED IMPORTED )

set_target_properties( gain_capital_ PROPERTIES IMPORTED_LOCATION ${PARENT_DIR}/lib/libgain_capital_api.so.1.0.0 )

target_link_libraries(${PROJECT_NAME} LINK_PUBLIC ${Boost_LIBRARIES} GTest::GTest GTest::Main gain_capital_)

# we cannot analyse results without gcov
find_program(GCOV_PATH gcov)
if(NOT GCOV_PATH)
  message(FATAL_ERROR "Code coverage analysis requires gcov!")
endif()

add_test(test_all unit_tests)