# Collect all source files (including subdirs)
file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS "*.cpp")

# The target is prefixed to avoid conflicts in the projects including EvoComb, but
# the library is exported as evocomb::core (file libevocomb_core)
add_library(evocomb_core SHARED ${CORE_SOURCES})
add_library(evocomb::core ALIAS evocomb_core)
set_target_properties(evocomb_core PROPERTIES
        EXPORT_NAME core
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        WINDOWS_EXPORT_ALL_SYMBOLS ON
)

# C++17 is required by the headers
target_compile_features(evocomb_core PUBLIC cxx_std_17)

# Include paths: the sources, or <prefix>/include/evocomb/core once installed
target_include_directories(evocomb_core PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/evocomb/core>
)

# Debug logs (the macros are in the headers)
if(EVOCOMB_DEBUG_LOG)
    target_compile_definitions(evocomb_core PUBLIC EVOCOMB_DEBUG_LOG)
endif()

# Threads (used by RThreadPool)
find_package(Threads REQUIRED)
target_link_libraries(evocomb_core PUBLIC Threads::Threads)
