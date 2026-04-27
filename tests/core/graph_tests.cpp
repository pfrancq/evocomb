/*

    EvoComb

    RGraphTests.cpp

    Tests of the graphs (edges, vertices and minimum spanning trees).

    Copyright 2002-2026 by Pascal Francq (pascal@francq.info).
    Copyright 2002-2008 by the Université Libre de Bruxelles (ULB).

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2.0 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library, as a file COPYING.LIB; if not, write
    to the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
    Boston, MA  02111-1307  USA

*/


//------------------------------------------------------------------------------
// include files for ANSI C/C++
#include <algorithm>
#include <vector>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "r_google_test.hpp"
#include "graph/r_graph.hpp"
#include "tools/r_exception.hpp"
using namespace evocomb;


//------------------------------------------------------------------------------
mLogModule("r.unit_tests");


//------------------------------------------------------------------------------
class RGraphTests : public RGoogleTest
{
};


//------------------------------------------------------------------------------
TEST_F(RGraphTests, build)
{
    RGraph graph(3);
    EXPECT_EQ(graph.nbVertices(), 3u);
    EXPECT_EQ(graph.addVertex(), 3u);
    const auto e01 = graph.addEdge(0, 1, 2.0);
    const auto e12 = graph.addEdge(1, 2, 1.0);
    graph.addEdge(1, 0, 3.0);                               // Parallel edge
    EXPECT_EQ(graph.nbEdges(), 3u);
    EXPECT_EQ(graph.findEdge(1, 0), e01);
    EXPECT_EQ(graph.findEdge(2, 1), e12);
    EXPECT_EQ(graph.findEdge(0, 3), cNoRef);
    EXPECT_EQ(graph.vertex(1).nbEdges(), 3u);
    EXPECT_EQ(graph.adjacentVertices(1), (std::vector<std::size_t>{0, 2}));
    EXPECT_EQ(graph.edge(e12).other(1), 2u);
    EXPECT_TRUE(graph.edge(e12).connects(2));
    EXPECT_THROW(graph.addEdge(0, 4, 1.0), RException);
}


//------------------------------------------------------------------------------
TEST_F(RGraphTests, remove)
{
    RGraph graph(4);
    graph.addEdge(0, 1, 1.0);                               // Edge 0
    graph.addEdge(1, 2, 2.0);                               // Edge 1
    graph.addEdge(2, 3, 3.0);                               // Edge 2

    // The last edge (2-3) takes the index 0
    graph.removeEdge(0);
    EXPECT_EQ(graph.nbEdges(), 2u);
    EXPECT_EQ(graph.findEdge(2, 3), 0u);
    EXPECT_EQ(graph.findEdge(0, 1), cNoRef);
    EXPECT_EQ(graph.vertex(0).nbEdges(), 0u);

    // The last vertex (3) takes the identifier 1
    graph.removeVertex(1);
    EXPECT_EQ(graph.nbVertices(), 3u);
    EXPECT_EQ(graph.nbEdges(), 1u);
    EXPECT_EQ(graph.vertex(1).id(), 1u);
    EXPECT_EQ(graph.findEdge(2, 1), 0u);
    EXPECT_DOUBLE_EQ(graph.edge(0).weight(), 3.0);

    // Reuse the graph
    graph.clear(2);
    EXPECT_EQ(graph.nbVertices(), 2u);
    EXPECT_EQ(graph.nbEdges(), 0u);
    EXPECT_EQ(graph.vertex(1).nbEdges(), 0u);
}


//------------------------------------------------------------------------------
TEST_F(RGraphTests, minSpanningTree)
{
    RGraph graph(4);
    graph.addEdge(0, 1, 2.0);
    graph.addEdge(1, 2, 1.0);
    graph.addEdge(0, 2, 5.0);
    graph.addEdge(2, 3, 1.5);
    std::vector<std::size_t> tree;
    EXPECT_DOUBLE_EQ(graph.minSpanningTree(tree), 4.5);
    std::sort(tree.begin(), tree.end());
    EXPECT_EQ(tree, (std::vector<std::size_t>{0, 1, 3}));

    // Tree as a graph
    RGraph mst;
    EXPECT_DOUBLE_EQ(graph.minSpanningTree(mst), 4.5);
    EXPECT_EQ(mst.nbVertices(), 4u);
    EXPECT_EQ(mst.nbEdges(), 3u);
    EXPECT_EQ(mst.findEdge(0, 2), cNoRef);
}


//------------------------------------------------------------------------------
TEST_F(RGraphTests, forest)
{
    // Two components: {0, 1, 2} and {3, 4}, plus an isolated vertex 5
    RGraph graph(6);
    graph.addEdge(0, 1, 4.0);
    graph.addEdge(1, 2, 1.0);
    graph.addEdge(0, 2, 2.0);
    graph.addEdge(3, 4, 7.0);
    std::vector<std::size_t> tree;
    EXPECT_DOUBLE_EQ(graph.minSpanningTree(tree), 10.0);
    EXPECT_EQ(tree.size(), 3u);
}
