# Evolutionary Combinatorial Optimization Library (EvoComb)

The EvoComb is a port of the R Library using the features of C++17.

When the original R Library was developed, the STL was unstable and lacked many important classes (such as vectors or
maps). Such tools were therefore integrated into the R Library. With time, the STL became more and more complete, but 
the algorithms of the R Library were still dependent on these tools.

The EvoComb is a complete port of the old code but based on the STL.

The classes are prefixed by `R` (for example `RMatrix`) and the files by `r_` (for example `r_matrix.hpp`), a
convention inherited from the original R Library.


## Build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
cmake --install build --prefix /usr/local     # optional
```

Options: `EVOCOMB_BUILD_TESTS`, `EVOCOMB_BUILD_EXAMPLES` and `EVOCOMB_INSTALL` (ON by
default when EvoComb is the main project, OFF when it is included by another
project), and `EVOCOMB_DEBUG_LOG` (OFF).

## Development container (Linux arm64)

`docker/Dockerfile` provides a Linux arm64 environment with GCC, Clang, CMake,
Ninja, ccache, gdb, lldb, valgrind, clang-tidy, cppcheck, lcov/gcovr, Doxygen
and Graphviz. The sources are mounted in `/evocomb` and the build directories are
in a Docker volume mounted in `/build`:

```sh
docker compose -f docker/compose.yaml build
docker compose -f docker/compose.yaml run --rm evocomb
# In the container
cmake -S /evocomb -B /build/debug -DCMAKE_BUILD_TYPE=Debug
cmake --build /build/debug
ctest --test-dir /build/debug --output-on-failure
valgrind --leak-check=full /build/debug/bin/unit_tests
```

## Using EvoComb in another project

EvoComb provides a CMake package with the imported targets `evocomb::core`,
`evocomb::optimization` and `evocomb::evocomb` (both libraries). They give the include
directories, the C++17 requirement and the dependencies:

```cmake
find_package(EvoComb 1.0 REQUIRED)
target_link_libraries(myapp PRIVATE evocomb::evocomb)
```

CMake finds the package after an installation (add the prefix to
`CMAKE_PREFIX_PATH` if it is not a standard one), or directly in a build
directory with `-DEvoComb_DIR=<evocomb build directory>`.

EvoComb can also be included in the build of the project (the tests and the
examples are then not built):

```cmake
add_subdirectory(path/to/evocomb evocomb)        # or FetchContent_Declare/MakeAvailable
target_link_libraries(myapp PRIVATE evocomb::evocomb)
```

The headers are included relatively to `src/core` and `src/optimization`, for example
`#include <matrix/r_matrix.hpp>` or `#include <ga/r_ga.hpp>`.

## Documentation

The API documentation is generated with Doxygen (and dot for the diagrams, if
it is installed) in `doc/html` during the build: `doc/html/index.html` links
to the documentation of `evocomb::core` and `evocomb::optimization`. It is only
regenerated when a header changes, and it can be disabled with
`-DEVOCOMB_BUILD_DOC=OFF`.

The CMake package gives the Doxygen tag files of EvoComb, so that the
documentation of a project links to the classes of evocomb:

```cmake
find_package(EvoComb 1.0 REQUIRED)
find_package(Doxygen)
set(DOXYGEN_TAGFILES ${EVOCOMB_DOXYGEN_TAGFILES})
doxygen_add_docs(mydoc ${PROJECT_SOURCE_DIR}/src)
```

See `cmake/EvoCombConfig.cmake.in` for the other variables (`EVOCOMB_DOC_FOUND`,
`EVOCOMB_DOC_DIR`, `EVOCOMB_DOXYGEN_TAGFILES_STRING` for a Doxyfile...).
