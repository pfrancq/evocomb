# Bin packing with the grouping genetic algorithm (GGA)
add_library(bin_packing STATIC
        bin_packing/r_bp_problem.cpp
        bin_packing/r_bp_gga.cpp
)
target_include_directories(bin_packing PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/bin_packing>
)
target_link_libraries(bin_packing PUBLIC evocomb::evocomb)

add_executable(bin_packing_example bin_packing/main.cpp)
target_link_libraries(bin_packing_example PRIVATE bin_packing)

# Hierarchy of objects with the hierarchical genetic algorithm (HGA)
add_library(hierarchy STATIC
        hierarchy/r_hierarchy_problem.cpp
        hierarchy/r_hierarchy_hga.cpp
)
target_include_directories(hierarchy PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/hierarchy>
)
target_link_libraries(hierarchy PUBLIC evocomb::evocomb)

add_executable(hierarchy_example hierarchy/main.cpp)
target_link_libraries(hierarchy_example PRIVATE hierarchy)

# VLSI placement with the 2D placement genetic algorithm (GA2D)
add_library(vlsi STATIC
        vlsi/r_vlsi_problem.cpp
)
target_include_directories(vlsi PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/vlsi>
)
target_link_libraries(vlsi PUBLIC evocomb::evocomb)

add_executable(vlsi_example vlsi/main.cpp)
target_link_libraries(vlsi_example PRIVATE vlsi)
