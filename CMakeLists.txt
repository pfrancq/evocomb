cmake_minimum_required(VERSION 3.15)

# Fix CMP0135 warning for FetchContent
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

# std::to_chars for real numbers (used by RLogger) requires macOS 13.3
if(APPLE AND NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "13.3" CACHE STRING "Minimum macOS version")
endif()

project(EvoComb VERSION 1.0.0 LANGUAGES CXX)

# Is EvoComb the main project (and not included by another project with
# add_subdirectory or FetchContent)?
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(EVOCOMB_MAIN_PROJECT ON)
else()
    set(EVOCOMB_MAIN_PROJECT OFF)
endif()

option(EVOCOMB_BUILD_TESTS "Build the unit tests" ${EVOCOMB_MAIN_PROJECT})
option(EVOCOMB_BUILD_EXAMPLES "Build the examples" ${EVOCOMB_MAIN_PROJECT})
option(EVOCOMB_INSTALL "Generate the install rules (library, headers and CMake package)" ${EVOCOMB_MAIN_PROJECT})
option(EVOCOMB_BUILD_DOC "Generate the documentation with Doxygen (if it is installed)" ${EVOCOMB_MAIN_PROJECT})

# Force C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Debug logs (LOG_DEBUG and LOG_FUNCTION), independent of the build type:
#   cmake -DEVOCOMB_DEBUG_LOG=ON ...
# (the definition is propagated to the projects using EvoComb, see core)
option(EVOCOMB_DEBUG_LOG "Compile the LOG_DEBUG and LOG_FUNCTION calls" OFF)

# Control where binaries and libs are built (only for the main project)
if(EVOCOMB_MAIN_PROJECT)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
endif()

# Install directories (lib, include, ...)
include(GNUInstallDirs)

# Add subdirectories in order of dependency
add_subdirectory(src/core)
add_subdirectory(src/optimization)

# Whole library: evocomb::evocomb links evocomb::core and evocomb::optimization
add_library(evocomb INTERFACE)
add_library(evocomb::evocomb ALIAS evocomb)
target_link_libraries(evocomb INTERFACE evocomb_core evocomb_optimization)

if(EVOCOMB_BUILD_TESTS)
    # Generate the header giving the root of the project to the tests
    configure_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/config.h.in"
            "${CMAKE_CURRENT_BINARY_DIR}/tests/generated/config.h"
    )
    add_subdirectory(tests)
endif()
if(EVOCOMB_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Documentation (in doc/html)
set(EVOCOMB_DOC_DIR ${PROJECT_SOURCE_DIR}/doc/html)
if(EVOCOMB_BUILD_DOC)
    include(cmake/EvoCombDoc.cmake)
endif()

#------------------------------------------------------------------------------
# CMake package: another project can use EvoComb with
#   find_package(EvoComb REQUIRED)
#   target_link_libraries(myapp PRIVATE evocomb::evocomb)
# either after "cmake --install" (CMAKE_PREFIX_PATH=<prefix>) or directly from
# the build directory (EvoComb_DIR=<build directory>).
include(CMakePackageConfigHelpers)
set(EVOCOMB_CMAKE_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/EvoComb)

set(EVOCOMB_DOC_INSTALL_DIR ${CMAKE_INSTALL_DOCDIR}/html)

# Configuration of the package in the build directory: the documentation is in
# the sources (doc/html)
set(EVOCOMB_CONFIG_DOC_DIR ${EVOCOMB_DOC_DIR})
configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/EvoCombConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/EvoCombConfig.cmake"
        INSTALL_DESTINATION "${CMAKE_CURRENT_BINARY_DIR}"
        INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}"
        PATH_VARS EVOCOMB_CONFIG_DOC_DIR
)

# Configuration of the installed package: the documentation is in
# <prefix>/share/doc/EvoComb/html
set(EVOCOMB_CONFIG_DOC_DIR ${EVOCOMB_DOC_INSTALL_DIR})
configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/EvoCombConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/install/EvoCombConfig.cmake"
        INSTALL_DESTINATION ${EVOCOMB_CMAKE_DIR}
        PATH_VARS EVOCOMB_CONFIG_DOC_DIR
)
write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/EvoCombConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
)

# Package usable from the build directory
export(TARGETS evocomb_core evocomb_optimization evocomb
        NAMESPACE evocomb::
        FILE "${CMAKE_CURRENT_BINARY_DIR}/EvoCombTargets.cmake"
)

if(EVOCOMB_INSTALL)
    # Libraries
    install(TARGETS evocomb_core evocomb_optimization evocomb
            EXPORT EvoCombTargets
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    # Headers (the directory structure is kept)
    install(DIRECTORY src/core/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/evocomb/core
            FILES_MATCHING PATTERN "*.hpp")
    install(DIRECTORY src/optimization/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/evocomb/optimization
            FILES_MATCHING PATTERN "*.hpp")

    # CMake package
    install(EXPORT EvoCombTargets
            NAMESPACE evocomb::
            DESTINATION ${EVOCOMB_CMAKE_DIR}
    )
    install(FILES
            "${CMAKE_CURRENT_BINARY_DIR}/install/EvoCombConfig.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/EvoCombConfigVersion.cmake"
            DESTINATION ${EVOCOMB_CMAKE_DIR}
    )

    # Documentation (if it was generated)
    if(EVOCOMB_BUILD_DOC)
        install(DIRECTORY ${EVOCOMB_DOC_DIR}/ DESTINATION ${EVOCOMB_DOC_INSTALL_DIR} OPTIONAL)
    endif()
endif()