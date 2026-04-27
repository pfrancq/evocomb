/*

    EvoComb

    RGA2DLayout.hpp

    Layout and Grid of the 2D Placement - Header.

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
#include <cstddef>
#include <cstdint>
#include <vector>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "ga2d/r_ga2d_problem.hpp"
#include "geometry/r_point.hpp"
#include "geometry/r_rect.hpp"
#include "tools/r_std.hpp"


//------------------------------------------------------------------------------
namespace evocomb {
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
 * The RGA2DShape class provides a shape to place: a set of disjoint rectangles and the size of their bounding
 * rectangle, which starts at (0,0). It references the rectangles of a configuration of an object or of a block (see
 * RGA2DBlock).
 * @short 2D Shape.
 */
struct RGA2DShape
{
    const std::vector<RRect>* rects = nullptr;  ///< Rectangles.
    RCoord width = 0;                       ///< Width.
    RCoord height = 0;                      ///< Height.
};


//------------------------------------------------------------------------------
/**
 * The RGA2DGrid class provides the occupation of the placement area: each unit square [x,x+1[ x [y,y+1[ is either free
 * or occupied by an object. It is used by the heuristics to test the positions quickly.
 * @short 2D Grid.
 */
class RGA2DGrid
{
public:

    /**
     * Construct the grid (all the squares are free).
     * @param width              Width.
     * @param height             Height.
     * @exception RException     The size is invalid or too large.
     */
    RGA2DGrid(RCoord width, RCoord height);

    /**
     * Get the width.
     */
    [[nodiscard]] RCoord width() const { return width_; }

    /**
     * Get the height.
     */
    [[nodiscard]] RCoord height() const { return height_; }

    /**
     * Free all the squares.
     */
    void clear();

    /**
     * Occupy the squares of some rectangles translated by a vector.
     * @param rects              Rectangles.
     * @param pos                Translation.
     * @param id                 Identifier of the object.
     * @exception RException     A rectangle is outside the grid.
     */
    void assign(const std::vector<RRect>& rects, const RPoint& pos, std::size_t id);

    /**
     * Look if a shape can be placed at a position: it must be inside the grid and all its squares must be free.
     * @param shape              Shape.
     * @param pos                Position (of the bottom-left corner of its bounding rectangle).
     */
    [[nodiscard]] bool fits(const RGA2DShape& shape, const RPoint& pos) const;

    /**
     * Look if a shape placed at a (valid) position can be moved by one unit. Only the squares entered by the move are
     * tested.
     * @param shape              Shape.
     * @param pos                Current position.
     * @param dx                 Move along X (-1, 0 or 1).
     * @param dy                 Move along Y (-1, 0 or 1, with dx or dy null).
     */
    [[nodiscard]] bool canMove(const RGA2DShape& shape, const RPoint& pos, RCoord dx, RCoord dy) const;

    /**
     * Look if a unit square is free (false if outside the grid).
     * @param x                  X coordinate.
     * @param y                  Y coordinate.
     */
    [[nodiscard]] bool isFree(RCoord x, RCoord y) const;

    /**
     * Get the object occupying a unit square.
     * @param x                  X coordinate.
     * @param y                  Y coordinate.
     * @return cNoRef if the square is free or outside the grid.
     */
    [[nodiscard]] std::size_t objAt(RCoord x, RCoord y) const;

private:

    /**
     * Look if the squares of a rectangle are free (the rectangle must be inside the grid).
     * @param rect               Rectangle.
     */
    [[nodiscard]] bool isFree(const RRect& rect) const;

    //-----------------------------------------------------
    RCoord width_;                          ///< Width.
    RCoord height_;                         ///< Height.
    std::vector<std::uint32_t> cells_;      ///< Squares (0 if free, identifier+1 otherwise).
};


//------------------------------------------------------------------------------
/**
 * The RGA2DLayout class provides a (partial) placement of the objects of a problem: for each object, its configuration
 * and its position (the bottom-left corner of the bounding rectangle of the configuration). It computes the geometric
 * measures (bounding rectangle, overlaps) and the lengths of the nets.
 *
 * The length of a net is the length of the minimum spanning tree connecting its placed connectors (the connectors of
 * the board are always placed), where the distance between two connectors is the smallest Manhattan distance between
 * the centers of their pins. A connector without pins is represented by the center of its object. The tree is computed
 * with a RGraph (complete graph over the placed connectors) kept per thread, so that several threads can compute
 * lengths on the same layout.
 *
 * The layout is a value: it can be copied (the problem is shared).
 * @short 2D Layout.
 */
class RGA2DLayout
{
public:

    /**
     * Placement of an object.
     */
    struct Placement
    {
        std::size_t config = cNoRef;        ///< Configuration (cNoRef if not placed).
        RPoint pos;                         ///< Position.
        std::size_t order = cNoRef;         ///< Order in which the object was placed.

        /**
         * Compare two placements (the order is ignored).
         * @param p              Placement.
         */
        [[nodiscard]] bool operator==(const Placement& p) const { return (config == p.config) && (pos == p.pos); }
    };

    /**
     * Construct an empty layout.
     */
    RGA2DLayout() = default;

    RGA2DLayout(const RGA2DLayout&) = default;
    RGA2DLayout(RGA2DLayout&&) noexcept = default;
    RGA2DLayout& operator=(const RGA2DLayout&) = default;
    RGA2DLayout& operator=(RGA2DLayout&&) noexcept = default;

    /**
     * Destruct the layout.
     */
    virtual ~RGA2DLayout() = default;

    /**
     * Initialize the layout for a problem (no object is placed).
     * @param problem            Problem (it must live as long as the layout).
     */
    void initLayout(const RGA2DProblem& problem);

    /**
     * Remove all the objects.
     */
    void clearLayout();

    /**
     * Get the problem.
     */
    [[nodiscard]] const RGA2DProblem& problem() const { return *problem_; }

    /**
     * Get the number of objects placed.
     */
    [[nodiscard]] std::size_t nbPlaced() const { return nbPlaced_; }

    /**
     * Look if an object is placed.
     * @param id                 Identifier of the object.
     */
    [[nodiscard]] bool isPlaced(const std::size_t id) const { return placements_[id].config != cNoRef; }

    /**
     * Get the placement of an object.
     * @param id                 Identifier of the object.
     */
    [[nodiscard]] const Placement& placement(const std::size_t id) const { return placements_[id]; }

    /**
     * Get the configuration of a placed object.
     * @param id                 Identifier of the object.
     */
    [[nodiscard]] const RGA2DConfig& config(const std::size_t id) const { return problem_->obj(id).configs()[placements_[id].config]; }

    /**
     * Place an object (or move it).
     * @param id                 Identifier of the object.
     * @param config             Configuration.
     * @param pos                Position.
     * @param order              Order of the placement (cNoRef to keep the current one).
     */
    void place(std::size_t id, std::size_t config, const RPoint& pos, std::size_t order = cNoRef);

    /**
     * Remove an object.
     * @param id                 Identifier of the object.
     */
    void unplace(std::size_t id);

    /**
     * Get the bounding rectangle of a placed object.
     * @param id                 Identifier of the object.
     */
    [[nodiscard]] RRect rect(std::size_t id) const;

    /**
     * Get the bounding rectangle of all the placed objects (empty if no object is placed).
     */
    [[nodiscard]] RRect boundary() const;

    /**
     * Compute the length of a net (see the class description).
     * @param net                Index of the net.
     */
    [[nodiscard]] double netLength(std::size_t net) const;

    /**
     * Compute the (possibly weighted) length of the nets connected to an object.
     * @param id                 Identifier of the object.
     * @param weighted           Multiply each length by the weight of the net?
     */
    [[nodiscard]] double objLength(std::size_t id, bool weighted) const;

    /**
     * Compute the (possibly weighted) length of all the nets.
     * @param weighted           Multiply each length by the weight of the net?
     */
    [[nodiscard]] double totalLength(bool weighted) const;

    /**
     * Compute the total weight of the nets connecting an object to at least one placed object or to the board.
     * @param id                 Identifier of the object.
     */
    [[nodiscard]] double connectedWeight(std::size_t id) const;

    /**
     * Verify the layout: valid configurations, objects inside the limits and no overlap.
     * @param complete           Must all the objects be placed?
     * @exception RGAException   The layout is invalid.
     */
    void verifyLayout(bool complete) const;

    /**
     * Look if two layouts place all the objects identically.
     * @param layout             Layout.
     */
    [[nodiscard]] bool sameLayout(const RGA2DLayout& layout) const { return placements_ == layout.placements_; }

private:

    /**
     * Get the centers of the pins of a connector of a net (in absolute coordinates).
     * @param item               Connector.
     * @param centers            Vector receiving the centers (emptied first).
     * @return false if the connector is not placed.
     */
    bool centers(const RGA2DNet::Item& item, std::vector<RPoint>& centers) const;

    //-----------------------------------------------------
    const RGA2DProblem* problem_ = nullptr; ///< Problem.
    std::vector<Placement> placements_;     ///< Placement of each object.
    std::size_t nbPlaced_ = 0;              ///< Number of objects placed.
};


}  //------- End of namespace evocomb ----------------------------------------------
