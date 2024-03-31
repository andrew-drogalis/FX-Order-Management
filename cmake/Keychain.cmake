
function(build_keychain SOURCE_BUILD PARENT_DIR)
    # For KeyChain 
    if (WIN32)
        target_sources(${SOURCE_BUILD}
            PRIVATE
                "${PARENT_DIR}/src/keychain/keychain_win.cpp")

        target_link_libraries(${SOURCE_BUILD}
            PRIVATE
                crypt32)
    elseif (APPLE)
        target_sources(${SOURCE_BUILD}
            PRIVATE
                "${PARENT_DIR}/src/keychain/keychain_mac.cpp")

        find_library(COREFOUNDATION_LIBRARY CoreFoundation REQUIRED)
        find_library(SECURITY_LIBRARY Security REQUIRED)

        target_link_libraries(${SOURCE_BUILD}
            PRIVATE
                ${COREFOUNDATION_LIBRARY}
                ${SECURITY_LIBRARY})
    else () # assuming Linux
        target_sources(${SOURCE_BUILD}
            PRIVATE
                "${PARENT_DIR}/src/keychain/keychain_linux.cpp")

        find_package(PkgConfig REQUIRED)
        pkg_check_modules(GLIB2 IMPORTED_TARGET glib-2.0)
        pkg_check_modules(LIBSECRET IMPORTED_TARGET libsecret-1)

        target_link_libraries(${SOURCE_BUILD}
            PRIVATE
                PkgConfig::GLIB2
                PkgConfig::LIBSECRET)
    endif ()
endfunction()