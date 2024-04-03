cmake_minimum_required(VERSION 3.20)
project(
  FX-Order-Management
  VERSION 1.0.0
  DESCRIPTION ""
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE "Debug")

include(GNUInstallDirs)

# Build Executable
add_executable(
  ${PROJECT_NAME}
  main.cpp
  src/fx_order_management.cpp
  src/trading_model.cpp
  src/fx_market_time.cpp
  src/fx_utilities.cpp)

set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION})

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# ------------------------------
# Build Configuration
include(cmake/CompilerWarnings.cmake)
include(cmake/Sanitizers.cmake)
include(cmake/StaticAnalyzers.cmake)
include(cmake/Tests.cmake)
include(cmake/Hardening.cmake)
include(cmake/PreventInSourceBuilds.cmake)
include(cmake/ClangFormat.cmake)
include(cmake/Keychain.cmake)

build_keychain(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR})
myproject_assure_out_of_source_builds()
myproject_set_project_warnings(
  ${PROJECT_NAME}
  TRUE
  "X"
  ""
  ""
  "X")
add_compile_options(-O2) # Required to Fortify Source
myproject_enable_hardening(${PROJECT_NAME} TRUE FALSE)
add_clang_format_target(RUN_CLANG-FORMAT ${CMAKE_CURRENT_SOURCE_DIR})

add_compile_options(-pipe -fPIC)
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  add_compile_options(-NDDEBUG)
endif()
if(CMAKE_BUILD_TYPE STREQUAL "Debug") 
  #myproject_enable_cppcheck(FALSE "X")
  #myproject_enable_clang_tidy(${PROJECT_NAME} FALSE)
  #myproject_enable_include_what_you_use()
  myproject_enable_coverage(${PROJECT_NAME})
  myproject_enable_sanitizers(
    ${PROJECT_NAME}
    TRUE
    TRUE
    TRUE
    FALSE
    TRUE)
endif()

# ------------------------------
# Libraries
include(FetchContent)
FetchContent_Declare(
  cpr
  GIT_REPOSITORY https://github.com/libcpr/cpr.git
  GIT_TAG 3020c34ae2b732121f37433e61599c34535e68a8
)# The commit hash for 1.10.x. Replace with the latest from: https://github.com/libcpr/cpr/releases
FetchContent_MakeAvailable(cpr)

target_link_libraries(${PROJECT_NAME} PRIVATE cpr::cpr)

find_package(
  Boost
  COMPONENTS program_options log log_setup
  REQUIRED)

include_directories(${Boost_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/lib)

add_library(gain_capital_api SHARED IMPORTED)

set_target_properties(gain_capital_api PROPERTIES IMPORTED_LOCATION
                                                  ${CMAKE_CURRENT_SOURCE_DIR}/lib/libgain_capital_api.so.1.0.0)

target_link_libraries(
  ${PROJECT_NAME}
  LINK_PUBLIC
  ${Boost_LIBRARIES}
  gain_capital_api)

# ------------------------------
# Testing
enable_testing()
add_subdirectory(test)
