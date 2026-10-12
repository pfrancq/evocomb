# evocomb::optimization {#mainpage}

The optimization library of EvoComb provides genetic
algorithms and multi-criteria decision methods. It is based on
[evocomb::core](../core/index.html). All the classes are in the `evocomb` namespace,
and the headers are included relatively to the `optimization` directory (for
example `#include <ga/r_ga.hpp>`).

| Directory       | Content |
|-----------------|---------|
| `ga`            | Generic genetic algorithm: instance, population and chromosomes, fitness strategies (direct or PROMETHEE), listeners. |
| `gga`           | Grouping genetic algorithm (GGA): groups, heuristics (first-fit) and operators. |
| `nngga`         | Nearest-neighbors grouping genetic algorithm (NNGGA). |
| `sgga`          | Similarity-based grouping genetic algorithm (SGGA). |
| `hga`           | Hierarchical genetic algorithm (HGA): trees of nodes containing objects. |
| `ga2d`          | 2D placement genetic algorithm (GA2D): objects, nets, layouts and placement heuristics (bottom-left, edge, center). |
| `multicriteria` | PROMETHEE method and 2-additive Choquet integral. |

The algorithms are configured with JSON (see evocomb::RJSON), and the populations
can be evaluated on several threads (see evocomb::RThreadPool).

## Using evocomb::optimization

```cmake
find_package(EvoComb 1.0 REQUIRED)
target_link_libraries(myapp PRIVATE evocomb::optimization)
```
