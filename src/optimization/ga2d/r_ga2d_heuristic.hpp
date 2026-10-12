/*

    EvoComb

    RGA2DHeuristic.hpp

    Placement Heuristics of the 2D Placement Genetic Algorithm - Header.

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
#include <string>
#include <vector>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "app/r_logger.hpp"
#include "ga/r_ga.hpp"
#include "ga/r_ga_fitness.hpp"
#include "ga2d/r_ga2d_context.hpp"
#include "ga2d/r_ga2d_layout.hpp"
#include "geometry/r_point.hpp"
#include "geometry/r_rect.hpp"
#include "tools/r_std.hpp"


//------------------------------------------------------------------------------
namespace evocomb {
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
 * The RGA2DBlock class provides a block of objects placed together as a single rigid shape, with their relative
 * positions and configurations (it is used by the crossover to keep a part of a parent, see RGA2DChromo).
 * @short 2D Block.
 */
struct RGA2DBlock
{
    /**
     * An object of the block.
     */
    struct Member
    {
        std::size_t obj;                    ///< Object.
        std::size_t config;                 ///< Configuration.
        RPoint pos;                         ///< Position relative to the block.
    };

    std::vector<Member> members;            ///< Objects.
    std::vector<RRect> rects;               ///< Rectangles of the objects (relative to the block).
    RCoord width = 0;                       ///< Width of the bounding rectangle.
    RCoord height = 0;                      ///< Height of the bounding rectangle.

    /**
     * Build a block from some objects placed in a layout.
     * @param layout             Layout.
     * @param objs               Objects (placed).
     */
    [[nodiscard]] static RGA2DBlock build(const RGA2DLayout& layout, const std::vector<std::size_t>& objs);

    /**
     * Get the shape of the block.
     */
    [[nodiscard]] RGA2DShape shape() const { return {&rects, width, height}; }
};


//------------------------------------------------------------------------------
/**
 * The RGA2DHeuristic class provides the base class for the placement heuristics. The objects (and the blocks) are
 * placed one by one:
 * 1. The next object is the one having the highest weight of nets connected
 *    to the objects already placed (or to the board). The first one is
 *    chosen randomly (or, if "first_random" is false, it is the object with
 *    the highest weight of nets).
 * 2. The heuristic proposes some valid positions for the object (for each
 *    orientation considered), with searchPositions. If none is found, the
 *    first position that fits (from the bottom-left corner) is used.
 * 3. The position is chosen with PROMETHEE II (see RGA2DContext).
 *
 * A heuristic has an internal state: an instance must not be used by two threads at the same time.
 * @short 2D Placement Heuristic.
 */
class RGA2DHeuristic
{
public:

    /**
     * Construct the heuristic.
     * @param logger             Logger.
     * @param name               Name.
     * @param context            Context (it must live as long as the heuristic).
     * @exception RException     The placement criteria are invalid.
     */
    RGA2DHeuristic(RLogger& logger, std::string name, const RGA2DContext& context);

    /**
     * Destruct the heuristic.
     */
    virtual ~RGA2DHeuristic() = default;

    /**
     * Get the name of the heuristic.
     */
    [[nodiscard]] const std::string& name() const { return name_; }

    /**
     * Get the context.
     */
    [[nodiscard]] const RGA2DContext& context() const { return context_; }

    /**
     * Place all the objects (the layout and the grid are emptied first).
     * @param logger             Logger.
     * @param layout             Layout.
     * @param grid               Grid (with the size of the limits).
     * @param blocks             Blocks to place as single shapes (their objects are not placed individually).
     * @param random             Random generator.
     * @exception RGAException   An object cannot be placed.
     */
    void run(RLogger& logger, RGA2DLayout& layout, RGA2DGrid& grid, const std::vector<RGA2DBlock>& blocks, RGARandom& random);

    //-----------------------------------------------------
    RGA2DHeuristic(const RGA2DHeuristic&) = delete;
    RGA2DHeuristic& operator=(const RGA2DHeuristic&) = delete;

protected:

    /**
     * Initialize the specific data of the heuristic (called by run).
     */
    virtual void init() {}

    /**
     * Propose some positions for a shape with addPosition.
     * @param shape              Shape.
     */
    virtual void searchPositions(const RGA2DShape& shape) = 0;

    /**
     * Method called after a shape was placed (before the bounding rectangle of the placement is updated).
     * @param shape              Shape.
     * @param pos                Position.
     */
    virtual void postPlace([[maybe_unused]] const RGA2DShape& shape, [[maybe_unused]] const RPoint& pos) {}

    /**
     * Propose a position for the shape currently treated. It is ignored if the shape does not fit at this position.
     * @param pos                Position.
     */
    void addPosition(const RPoint& pos);

    /**
     * Push a shape (placed at a valid position) to the bottom and to the left as long as possible.
     * @param shape              Shape.
     * @param pos                Position (updated).
     */
    void pushBottomLeft(const RGA2DShape& shape, RPoint& pos) const;

    /**
     * Push a shape (placed at a valid position) in the direction of the center of the limits as long as possible.
     * @param shape              Shape.
     * @param pos                Position (updated).
     */
    void pushCenter(const RGA2DShape& shape, RPoint& pos) const;

    /**
     * Get the grid.
     */
    [[nodiscard]] const RGA2DGrid& grid() const { return *grid_; }

    /**
     * Get the layout.
     */
    [[nodiscard]] const RGA2DLayout& layout() const { return *layout_; }

    //-----------------------------------------------------
    RCoord width_ = 0;                      ///< Width of the limits.
    RCoord height_ = 0;                     ///< Height of the limits.
    std::size_t nbPlaced_ = 0;              ///< Number of shapes placed.
    RRect result_;                          ///< Bounding rectangle of the placed objects.

private:

    /**
     * An element to place: an object or a block.
     */
    struct Item
    {
        std::size_t obj = cNoRef;           ///< Object (cNoRef for a block).
        const RGA2DBlock* block = nullptr;  ///< Block.
    };

    /**
     * A valid position.
     */
    struct Candidate
    {
        std::size_t config;                 ///< Configuration (cNoRef for a block).
        RPoint pos;                         ///< Position.
        std::vector<double> criteria;       ///< Area and distance.
    };

    /**
     * Select the next item to place, and put it at the position nbPlaced_ of items_.
     */
    void selectNext();

    /**
     * Get the weight of the nets of an item.
     * @param item               Item.
     * @param connected          Only the nets connected to placed objects or to the board?
     */
    [[nodiscard]] double weight(const Item& item, bool connected) const;

    /**
     * Get the shape of an item in a given configuration.
     * @param item               Item.
     * @param config             Configuration.
     */
    [[nodiscard]] RGA2DShape shape(const Item& item, std::size_t config) const;

    /**
     * Place an item in the layout (and possibly in the grid).
     * @param item               Item.
     * @param config             Configuration.
     * @param pos                Position.
     * @param grid               Update the grid?
     */
    void place(const Item& item, std::size_t config, const RPoint& pos, bool grid);

    /**
     * Remove an item from the layout.
     * @param item               Item.
     */
    void unplace(const Item& item);

    /**
     * Place the current item (see the class description).
     * @param logger             Logger.
     */
    void placeCurrent(RLogger& logger);

    //-----------------------------------------------------
    std::string name_;                      ///< Name.
    const RGA2DContext& context_;           ///< Context.
    RGAPromFitness prom_;                   ///< PROMETHEE kernel choosing the positions.
    RGA2DLayout* layout_ = nullptr;         ///< Current layout.
    RGA2DGrid* grid_ = nullptr;             ///< Current grid.
    std::vector<Item> items_;               ///< Items to place.
    std::size_t curConfig_ = cNoRef;        ///< Configuration currently tried.
    RGA2DShape curShape_;                   ///< Shape currently tried.
    std::vector<Candidate> candidates_;     ///< Valid positions.
    std::vector<std::size_t> nets_;         ///< Temporary nets.
    std::vector<const std::vector<double>*> sols_;  ///< Temporary solutions.
    std::vector<double> fi_;                ///< Temporary Fi.
};


//------------------------------------------------------------------------------
/**
 * The RGA2DBottomLeftHeuristic class provides the "bottom_left" heuristic:
 * the objects are placed in columns from the bottom-left corner, and pushed to the bottom and to the left. When the
 * objects exceed the width of the limits, a new row of columns is started above the current placement.
 * @short Bottom-Left Heuristic.
 */
class RGA2DBottomLeftHeuristic : public RGA2DHeuristic
{
public:

    /**
     * Construct the heuristic.
     * @param logger             Logger.
     * @param context            Context.
     */
    RGA2DBottomLeftHeuristic(RLogger& logger, const RGA2DContext& context);

protected:

    void init() override;
    void searchPositions(const RGA2DShape& shape) override;
    void postPlace(const RGA2DShape& shape, const RPoint& pos) override;

private:

    //-----------------------------------------------------
    RPoint nextObj_;                        ///< Position of the next column.
    RPoint nextLine_;                       ///< Position in the current column.
};


//------------------------------------------------------------------------------
/**
 * The RGA2DEdgeHeuristic class provides the "edge" heuristic: the objects are placed in levels from the bottom-left
 * corner (as with RGA2DBottomLeftHeuristic), but the placement alternately grows in width and in height to keep a shape
 * proportional to the limits.
 * @short Edge Heuristic.
 */
class RGA2DEdgeHeuristic : public RGA2DHeuristic
{
public:

    /**
     * Construct the heuristic.
     * @param logger             Logger.
     * @param context            Context.
     */
    RGA2DEdgeHeuristic(RLogger& logger, const RGA2DContext& context);

protected:

    void init() override;
    void searchPositions(const RGA2DShape& shape) override;
    void postPlace(const RGA2DShape& shape, const RPoint& pos) override;

private:

    /**
     * Get a level (the vector grows if necessary).
     * @param idx                Index.
     */
    RPoint& level(std::size_t idx);

    //-----------------------------------------------------
    std::vector<RPoint> levels_;            ///< Levels.
    std::size_t nbLevels_ = 0;              ///< Number of levels.
    std::size_t curLevel_ = 0;              ///< Current level.
    RPoint actual_;                         ///< Current position.
    RPoint max_;                            ///< Maximal position.
    RPoint last_;                           ///< Last position.
};


//------------------------------------------------------------------------------
/**
 * The RGA2DCenterHeuristic class provides the "center" heuristic: the first object is placed at the center of the
 * limits, and the next ones around the placed objects: at each corner of the union of the placed objects, the four
 * positions touching the corner are tried, and the object is then pushed in the direction of the center.
 * @short Center Heuristic.
 */
class RGA2DCenterHeuristic : public RGA2DHeuristic
{
public:

    /**
     * Construct the heuristic.
     * @param logger             Logger.
     * @param context            Context.
     */
    RGA2DCenterHeuristic(RLogger& logger, const RGA2DContext& context);

    /**
     * Compute the corners of the union of some rectangles: the points where the boundary of the union changes of
     * direction.
     * @param rects              Rectangles.
     */
    [[nodiscard]] static std::vector<RPoint> corners(const std::vector<RRect>& rects);

protected:

    void init() override;
    void searchPositions(const RGA2DShape& shape) override;
    void postPlace(const RGA2DShape& shape, const RPoint& pos) override;

private:

    //-----------------------------------------------------
    std::vector<RRect> placed_;             ///< Rectangles of the placed objects.
    std::vector<RPoint> corners_;           ///< Corners of their union.
};


}  //------- End of namespace evocomb ----------------------------------------------
