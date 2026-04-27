include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
)
# For Windows compatibility
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Define your test executable
file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS "*.cpp")
add_executable(unit_tests ${TEST_SOURCES})

target_include_directories(unit_tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}/generated"
)

# Link against the libraries and GTest
target_link_libraries(unit_tests PRIVATE evocomb::evocomb bin_packing hierarchy vlsi GTest::gtest_main)

# Enable CTest discovery
include(GoogleTest)
gtest_discover_tests(unit_tests)