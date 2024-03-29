cmake_minimum_required(VERSION 3.20)
project(FX-Order-Management
        VERSION 1.0.0
        DESCRIPTION ""
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(GNUInstallDirs)

# Build Executable
add_executable(${PROJECT_NAME} main.cpp
                src/fx_order_management.cpp
                src/trading_model.cpp
                src/fx_market_time.cpp
                src/fx_utilities.cpp)

set_target_properties(${PROJECT_NAME} PROPERTIES 
                        VERSION ${PROJECT_VERSION})

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

include(FetchContent)
FetchContent_Declare(cpr GIT_REPOSITORY https://github.com/libcpr/cpr.git
                         GIT_TAG 3020c34ae2b732121f37433e61599c34535e68a8) # The commit hash for 1.10.x. Replace with the latest from: https://github.com/libcpr/cpr/releases
FetchContent_MakeAvailable(cpr)

target_link_libraries(${PROJECT_NAME} PRIVATE cpr::cpr)

find_package(Boost COMPONENTS program_options log log_setup REQUIRED)

include_directories( ${Boost_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/lib)

add_library( gain_capital_api SHARED IMPORTED )

set_target_properties( gain_capital_api PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/lib/libgain_capital_api.so.1.2.0 )

target_link_libraries(${PROJECT_NAME} LINK_PUBLIC ${Boost_LIBRARIES} gain_capital_api)

# For KeyChain 
if (WIN32)

    target_sources(${PROJECT_NAME}
        PRIVATE
            "src/keychain/keychain_win.cpp")

    target_link_libraries(${PROJECT_NAME}
        PRIVATE
            crypt32)
elseif (APPLE)

    target_sources(${PROJECT_NAME}
        PRIVATE
            "src/keychain/keychain_mac.cpp")

    find_library(COREFOUNDATION_LIBRARY CoreFoundation REQUIRED)
    find_library(SECURITY_LIBRARY Security REQUIRED)

    target_link_libraries(${PROJECT_NAME}
        PRIVATE
            ${COREFOUNDATION_LIBRARY}
            ${SECURITY_LIBRARY})
else () # assuming Linux

    target_sources(${PROJECT_NAME}
        PRIVATE
            "src/keychain/keychain_linux.cpp")

    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GLIB2 IMPORTED_TARGET glib-2.0)
    pkg_check_modules(LIBSECRET IMPORTED_TARGET libsecret-1)

    target_link_libraries(${PROJECT_NAME}
        PRIVATE
            PkgConfig::GLIB2
            PkgConfig::LIBSECRET)
endif ()



