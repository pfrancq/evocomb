# Collect all source files (including subdirs)
file(GLOB_RECURSE GA_SOURCES CONFIGURE_DEPENDS "*.cpp")

# The target is prefixed to avoid conflicts in the projects including EvoComb, but
# the library is exported as evocomb::optimization (file libevocomb_optimization)
add_library(evocomb_optimization SHARED ${GA_SOURCES})
add_library(evocomb::optimization ALIAS evocomb_optimization)
set_target_properties(evocomb_optimization PROPERTIES
        EXPORT_NAME optimization
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        WINDOWS_EXPORT_ALL_SYMBOLS ON
)

target_link_libraries(evocomb_optimization PUBLIC evocomb_core)

# The installed library finds evocomb_core in its own directory
if(APPLE)
    set_target_properties(evocomb_optimization PROPERTIES INSTALL_RPATH "@loader_path")
elseif(UNIX)
    set_target_properties(evocomb_optimization PROPERTIES INSTALL_RPATH "$ORIGIN")
endif()

# Include paths: the sources, or <prefix>/include/evocomb/optimization once installed
target_include_directories(evocomb_optimization PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/evocomb/optimization>
)
