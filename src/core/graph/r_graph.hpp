/*

    EvoComb

    RGraph.hpp

    Graph (vertices, weighted edges and minimum spanning tree) - Header.

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
#pragma once


//------------------------------------------------------------------------------
// include files for ANSI C/C++
#include <algorithm>
#include <cstddef>
#include <functional>
#include <queue>
#include <type_traits>
#include <utility>
#include <vector>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "app/r_logger.hpp"
#include "tools/r_exception.hpp"
#include "tools/r_std.hpp"


//------------------------------------------------------------------------------
namespace evocomb {
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// forward declaration
template<class V, class E> class RGenericGraph;


//------------------------------------------------------------------------------
/**
 * The RVertex class provides the base class for the vertices of a graph (see RGenericGraph). A vertex is identified by
 * its index in the graph and knows the edges connected to it. A derived class can add its own data (it must be
 * constructible from an identifier).
 * @short Vertex.
 */
class RVertex
{
public:

    /**
     * Construct the vertex.
     * @param id                 Identifier (index in the graph).
     */
    explicit RVertex(const std::size_t id) : id_(id) {}

    /**
     * Get the identifier of the vertex.
     */
    [[nodiscard]] std::size_t id() const { return id_; }

    /**
     * Get the indexes of the edges connected to the vertex.
     */
    [[nodiscard]] const std::vector<std::size_t>& edges() const { return edges_; }

    /**
     * Get the number of edges connected to the vertex.
     */
    [[nodiscard]] std::size_t nbEdges() const { return edges_.size(); }

private:

    //-----------------------------------------------------
    std::size_t id_;                        ///< Identifier.
    std::vector<std::size_t> edges_;        ///< Edges.

    //-----------------------------------------------------
    template<class, class> friend class RGenericGraph;
};


//------------------------------------------------------------------------------
/**
 * The REdge class provides the base class for the edges of a graph (see RGenericGraph): an undirected edge between two
 * vertices, with a weight. A derived class can add its own data (it must be constructible from the two vertices and the
 * weight).
 * @short Edge.
 */
class REdge
{
public:

    /**
     * Construct the edge.
     * @param from               First vertex.
     * @param to                 Second vertex.
     * @param weight             Weight.
     */
    REdge(const std::size_t from, const std::size_t to, const double weight) : from_(from), to_(to), weight_(weight) {}

    /**
     * Get the first vertex.
     */
    [[nodiscard]] std::size_t from() const { return from_; }

    /**
     * Get the second vertex.
     */
    [[nodiscard]] std::size_t to() const { return to_; }

    /**
     * Get the weight.
     */
    [[nodiscard]] double weight() const { return weight_; }

    /**
     * Set the weight.
     * @param weight             Weight.
     */
    void weight(const double weight) { weight_ = weight; }

    /**
     * Look if the edge is connected to a vertex.
     * @param id                 Identifier of the vertex.
     */
    [[nodiscard]] bool connects(const std::size_t id) const { return from_ == id || to_ == id; }

    /**
     * Get the other end of the edge.
     * @param id                 Identifier of one of the vertices.
     */
    [[nodiscard]] std::size_t other(const std::size_t id) const { return from_ == id ? to_ : from_; }

private:

    //-----------------------------------------------------
    std::size_t from_;                      ///< First vertex.
    std::size_t to_;                        ///< Second vertex.
    double weight_;                         ///< Weight.

    //-----------------------------------------------------
    template<class, class> friend class RGenericGraph;
};


//------------------------------------------------------------------------------
/**
 * The RGenericGraph class provides an undirected graph with weighted edges. The vertices and the edges are identified
 * by their indexes, so that the graph is a value (it can be copied). The classes of the vertices and of the edges can
 * be derived from RVertex and REdge to add some data:
 * @code
 * evocomb::RGraph graph(4);                        // Vertices 0 to 3
 * graph.addEdge(0, 1, 2.0);
 * graph.addEdge(1, 2, 1.0);
 * graph.addEdge(0, 2, 5.0);
 * graph.addEdge(2, 3, 1.5);
 * std::vector<std::size_t> tree;
 * double weight = graph.minSpanningTree(tree); // 4.5 (edges 0, 1 and 3)
 * @endcode
 * The graph can be emptied with clear, which keeps the allocated memory: a graph used as a working structure can be
 * reused without allocation.
 * @tparam V                     Class of the vertices.
 * @tparam E                     Class of the edges.
 * @short Graph.
 */
template<class V = RVertex, class E = REdge>
    class RGenericGraph
{
    static_assert(std::is_base_of_v<RVertex, V>, "V must inherit from RVertex");
    static_assert(std::is_base_of_v<REdge, E>, "E must inherit from REdge");

public:

    /**
     * Construct a graph.
     * @param nbVertices         Number of vertices created.
     */
    explicit RGenericGraph(const std::size_t nbVertices = 0)
    {
        clear(nbVertices);
    }

    /**
     * Remove all the edges and all the vertices, and create new vertices. The memory allocated is kept.
     * @param nbVertices         Number of vertices created.
     */
    void clear(const std::size_t nbVertices = 0)
    {
        edges_.clear();
        if constexpr (std::is_same_v<V, RVertex>)
        {
            // Reuse the existing vertices (and their lists of edges)
            if (vertices_.size() > nbVertices)
                vertices_.erase(vertices_.begin() + static_cast<std::ptrdiff_t>(nbVertices), vertices_.end());
            for (auto& vertex : vertices_)
                vertex.edges_.clear();
        }
        else
            vertices_.clear();
        for (std::size_t i = vertices_.size(); i < nbVertices; ++i)
            vertices_.emplace_back(i);
    }

    /**
     * Get the number of vertices.
     */
    [[nodiscard]] std::size_t nbVertices() const { return vertices_.size(); }

    /**
     * Get the number of edges.
     */
    [[nodiscard]] std::size_t nbEdges() const { return edges_.size(); }

    /**
     * Get the vertices.
     */
    [[nodiscard]] const std::vector<V>& vertices() const { return vertices_; }

    /**
     * Get the edges.
     */
    [[nodiscard]] const std::vector<E>& edges() const { return edges_; }

    /**
     * Get a vertex.
     * @param id                 Identifier.
     */
    [[nodiscard]] V& vertex(const std::size_t id) { return vertices_[id]; }

    /**
     * Get a vertex.
     * @param id                 Identifier.
     */
    [[nodiscard]] const V& vertex(const std::size_t id) const { return vertices_[id]; }

    /**
     * Get an edge.
     * @param id                 Index.
     */
    [[nodiscard]] E& edge(const std::size_t id) { return edges_[id]; }

    /**
     * Get an edge.
     * @param id                 Index.
     */
    [[nodiscard]] const E& edge(const std::size_t id) const { return edges_[id]; }

    /**
     * Add a vertex.
     * @return the identifier of the vertex.
     */
    std::size_t addVertex()
    {
        vertices_.emplace_back(vertices_.size());
        return vertices_.size() - 1;
    }

    /**
     * Add an edge.
     * @param from               First vertex.
     * @param to                 Second vertex.
     * @param weight             Weight.
     * @return the index of the edge.
     * @exception RException     A vertex does not exist.
     */
    std::size_t addEdge(const std::size_t from, const std::size_t to, const double weight)
    {
        mLogModule("r.core");
        if (from >= vertices_.size() || to >= vertices_.size())
            mThrowRException("Invalid edge between vertices ", from, " and ", to, " (", vertices_.size(), " vertices)");
        const auto id = edges_.size();
        edges_.emplace_back(from, to, weight);
        vertices_[from].edges_.push_back(id);
        if (to != from)
            vertices_[to].edges_.push_back(id);
        return id;
    }

    /**
     * Find an edge between two vertices.
     * @param id1                First vertex.
     * @param id2                Second vertex.
     * @return the index of the edge, or cNoRef if there is none.
     */
    [[nodiscard]] std::size_t findEdge(const std::size_t id1, const std::size_t id2) const
    {
        for (const auto e : vertices_[id1].edges_)
            if (edges_[e].other(id1) == id2)
                return e;
        return cNoRef;
    }

    /**
     * Get the vertices adjacent to a vertex (without duplicates, in the order of the edges).
     * @param id                 Vertex.
     */
    [[nodiscard]] std::vector<std::size_t> adjacentVertices(const std::size_t id) const
    {
        std::vector<std::size_t> res;
        for (const auto e : vertices_[id].edges_)
            if (const auto other = edges_[e].other(id); std::find(res.begin(), res.end(), other) == res.end())
                res.push_back(other);
        return res;
    }

    /**
     * Remove an edge. The last edge takes its index.
     * @param id                 Index of the edge.
     */
    void removeEdge(const std::size_t id)
    {
        detach(id);
        const auto last = edges_.size() - 1;
        if (id != last)
        {
            // The last edge takes the index of the removed one
            edges_[id] = std::move(edges_[last]);
            for (const auto v : {edges_[id].from_, edges_[id].to_})
                std::replace(vertices_[v].edges_.begin(), vertices_[v].edges_.end(), last, id);
        }
        edges_.pop_back();
    }

    /**
     * Remove a vertex and its edges. The last vertex takes its identifier.
     * @param id                 Identifier of the vertex.
     */
    void removeVertex(const std::size_t id)
    {
        while (!vertices_[id].edges_.empty())
            removeEdge(vertices_[id].edges_.back());
        const auto last = vertices_.size() - 1;
        if (id != last)
        {
            // The last vertex takes the identifier of the removed one
            for (const auto e : vertices_[last].edges_)
            {
                if (edges_[e].from_ == last)
                    edges_[e].from_ = id;
                if (edges_[e].to_ == last)
                    edges_[e].to_ = id;
            }
            vertices_[id] = std::move(vertices_[last]);
            vertices_[id].id_ = id;
        }
        vertices_.pop_back();
    }

    /**
     * Compute a minimum spanning tree (Prim's algorithm). If the graph is not connected, a minimum spanning forest is
     * computed.
     * @param tree               Vector receiving the indexes of the edges of the tree (emptied first).
     * @return the total weight of the tree.
     */
    double minSpanningTree(std::vector<std::size_t>& tree) const
    {
        using Item = std::pair<double, std::size_t>;       // (weight, edge)
        tree.clear();
        std::vector<char> inTree(vertices_.size(), 0);
        std::priority_queue<Item, std::vector<Item>, std::greater<>> queue;
        double total = 0.0;
        for (std::size_t start = 0; start < vertices_.size(); ++start)
        {
            if (inTree[start])
                continue;
            const auto add = [&](const std::size_t v) {
                inTree[v] = 1;
                for (const auto e : vertices_[v].edges_)
                    if (!inTree[edges_[e].other(v)])
                        queue.emplace(edges_[e].weight_, e);
            };
            add(start);
            while (!queue.empty())
            {
                const auto [weight, e] = queue.top();
                queue.pop();
                const auto& edge = edges_[e];
                const auto next = inTree[edge.from_] ? edge.to_ : edge.from_;
                if (inTree[next])
                    continue;
                tree.push_back(e);
                total += weight;
                add(next);
            }
        }
        return total;
    }

    /**
     * Compute a minimum spanning tree as a new graph with the same vertices (see minSpanningTree).
     * @param tree               Graph receiving the tree (emptied first).
     * @return the total weight of the tree.
     */
    double minSpanningTree(RGenericGraph& tree) const
    {
        std::vector<std::size_t> edges;
        const auto total = minSpanningTree(edges);
        tree.clear(vertices_.size());
        for (const auto e : edges)
            tree.addEdge(edges_[e].from_, edges_[e].to_, edges_[e].weight_);
        return total;
    }

private:

    /**
     * Remove an edge from the lists of its vertices.
     * @param id                 Index of the edge.
     */
    void detach(const std::size_t id)
    {
        for (const auto v : {edges_[id].from_, edges_[id].to_})
        {
            auto& list = vertices_[v].edges_;
            if (const auto it = std::find(list.begin(), list.end(), id); it != list.end())
                list.erase(it);
        }
    }

    //-----------------------------------------------------
    std::vector<V> vertices_;               ///< Vertices.
    std::vector<E> edges_;                  ///< Edges.
};


//------------------------------------------------------------------------------
/**
 * Graph with the default vertices and edges.
 */
using RGraph = RGenericGraph<>;


}  //------- End of namespace evocomb ----------------------------------------------
