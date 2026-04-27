# evocomb::core {#mainpage}

The core library of EvoComb provides the basic tools used by
the other libraries. All the classes are in the `evocomb` namespace, and the
headers are included relatively to the `core` directory (for example
`#include <matrix/r_matrix.hpp>`).

| Directory  | Content |
|------------|---------|
| `app`      | Logging (evocomb::RLogger and the `LOG_*` macros). |
| `tools`    | Exceptions (evocomb::RException), JSON values (evocomb::RJSON), pool of threads (evocomb::RThreadPool), string tools and common definitions (`cNoRef`...). |
| `io`       | Files (evocomb::RIOFile), text and JSON parsers (evocomb::RTextParser, evocomb::RJSONParser). |
| `matrix`   | Vectors and matrices: full (evocomb::RMatrix), triangular and symmetric, sparse (evocomb::RSparseMatrix, evocomb::RSparseSymmetricMatrix), ordered by decreasing values (evocomb::RMaxMatrix), and matrices stored on disk (evocomb::RMatrixStorage). |
| `graph`    | Graphs with weighted edges and minimum spanning trees (evocomb::RGraph). |
| `geometry` | Points, rectangles and rectilinear polygons (evocomb::RPoint, evocomb::RRect, evocomb::RPolygon). |

## Using evocomb::core

```cmake
find_package(EvoComb 1.0 REQUIRED)
target_link_libraries(myapp PRIVATE evocomb::core)
```

See also the documentation of [evocomb::optimization](../optimization/index.html).
