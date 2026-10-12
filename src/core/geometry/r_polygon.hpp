/*

    EvoComb

    RPolygon.hpp

    Rectilinear polygon with integer coordinates - Header.

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
#include <array>
#include <cstddef>
#include <string>
#include <vector>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "geometry/r_point.hpp"
#include "geometry/r_rect.hpp"


//------------------------------------------------------------------------------
namespace evocomb {
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
 * Orientations of a shape: the identity, the mirrors and the rotation of 90 degrees (counter-clockwise) combined with
 * the mirrors. The mirror "X" is the horizontal mirror (the Y coordinates are negated) and the mirror "Y" the vertical
 * one (the X coordinates are negated). The mirrors are applied before the rotation.
 */
enum class ROrientation
{
    Normal,                                 ///< Identity.
    MirrorX,                                ///< Horizontal mirror.
    MirrorY,                                ///< Vertical mirror.
    MirrorXY,                               ///< Both mirrors (rotation of 180 degrees).
    Rota90,                                 ///< Rotation of 90 degrees.
    Rota90MirrorX,                          ///< Horizontal mirror, then rotation.
    Rota90MirrorY,                          ///< Vertical mirror, then rotation.
    Rota90MirrorXY                          ///< Both mirrors, then rotation.
};


//------------------------------------------------------------------------------
/**
 * All the orientations (in the order of their values).
 */
inline constexpr std::array<ROrientation, 8> cOrientations = {
    ROrientation::Normal, ROrientation::MirrorX, ROrientation::MirrorY, ROrientation::MirrorXY,
    ROrientation::Rota90, ROrientation::Rota90MirrorX, ROrientation::Rota90MirrorY, ROrientation::Rota90MirrorXY
};


//------------------------------------------------------------------------------
/**
 * The RPolygon class provides a rectilinear polygon (all its edges are horizontal or vertical) whose vertices have
 * integer coordinates.
 *
 * As for RRect, the plane is seen as a grid of unit squares, and the vertices are the corners of these squares (the
 * lines between them), not the squares themselves. The unit square (x,y) is the square [x,x+1[ x [y,y+1[, i.e. the
 * one whose bottom-left corner is (x,y). An edge of the polygon is therefore a line between two rows (or two columns)
 * of unit squares: the squares on one side are inside the polygon, those on the other side are outside. No +1 or -1
 * is needed to describe a shape: to cover the columns of squares 0 to 9, the vertical edges are at x=0 and x=10. The
 * polygon (0,0) (10,0) (10,5) (0,5) covers thus 10 x 5 unit squares (it is RRect(0,0,10,5)), and not 11 x 6, while
 * the polygon (0,0) (9,0) (9,4) (0,4) covers only 9 x 4 unit squares.
 *
 * For a more complex shape, the simplest way is to follow the outline of the covered squares. For example, an "L"
 * whose vertical bar covers the columns 0 to 3 of the rows 0 to 9, and whose horizontal bar covers the columns 0 to 9
 * of the rows 0 to 3:
 * @code
 *   y
 *  10 +-------+
 *     |       |
 *     |       |
 *     |       |
 *   4 |       +-----------+
 *     |                   |
 *   0 +-------------------+
 *     0       4          10   x
 * @endcode
 * is given by the vertices (0,0) (10,0) (10,4) (4,4) (4,10) (0,10), and not by (0,0) (9,0) (9,3) (3,3) (3,9) (0,9)
 * (the last and first squares of each row or column). At the inner corner (4,4), the unit square (3,3) is inside the
 * polygon while the unit square (4,4) is outside. This polygon covers 64 unit squares and is decomposed (see rects)
 * into [0,0;10,4[ and [0,4;4,10[. As for rectangles, two polygons sharing an edge (for example a second polygon
 * starting at x=10) are adjacent and do not overlap.
 *
 * The polygon can be decomposed into rectangles (see rects) and transformed according to an orientation (see
 * transformed).
 * @short Rectilinear Polygon.
 */
class RPolygon
{
public:

    /**
     * Construct an empty polygon.
     */
    RPolygon() = default;

    /**
     * Construct a polygon from its vertices. The last vertex can repeat the first one, and the consecutive duplicate or
     * aligned vertices are removed.
     * @param vertices           Vertices, i.e. corners of the unit squares (in any direction).
     */
    explicit RPolygon(std::vector<RPoint> vertices);

    /**
     * Construct a rectangular polygon.
     * @param rect               Rectangle.
     */
    explicit RPolygon(const RRect& rect);

    /**
     * Get the vertices.
     */
    [[nodiscard]] const std::vector<RPoint>& vertices() const { return vertices_; }

    /**
     * Get the number of vertices.
     */
    [[nodiscard]] std::size_t size() const { return vertices_.size(); }

    /**
     * Look if the polygon is valid: at least four vertices, and all the edges horizontal or vertical.
     */
    [[nodiscard]] bool isRectilinear() const;

    /**
     * Look if the polygon is a rectangle.
     */
    [[nodiscard]] bool isRect() const { return vertices_.size() == 4 && isRectilinear(); }

    /**
     * Get the bounding rectangle.
     */
    [[nodiscard]] RRect boundary() const;

    /**
     * Get the area.
     */
    [[nodiscard]] RCoord area() const;

    /**
     * Decompose the polygon into disjoint rectangles covering it exactly.
     * @exception RException     The polygon is not rectilinear.
     */
    [[nodiscard]] std::vector<RRect> rects() const;

    /**
     * Get the polygon translated by a vector.
     * @param pt                 Vector.
     */
    [[nodiscard]] RPolygon translated(const RPoint& pt) const;

    /**
     * Get the polygon transformed according to an orientation, and then translated so that its bounding rectangle
     * starts at (0,0).
     * @param ori                Orientation.
     * @param offset             Translation applied after the transformation (to apply the same transformation to other
     *                           elements, see transform).
     */
    [[nodiscard]] RPolygon transformed(ROrientation ori, RPoint& offset) const;

    /**
     * Transform a point (a vector) according to an orientation.
     * @param pt                 Point.
     * @param ori                Orientation.
     */
    [[nodiscard]] static RPoint transform(const RPoint& pt, ROrientation ori);

    /**
     * Transform a rectangle according to an orientation.
     * @param rect               Rectangle.
     * @param ori                Orientation.
     */
    [[nodiscard]] static RRect transform(const RRect& rect, ROrientation ori);

    /**
     * Compare two polygons (same vertices in the same order).
     * @param poly               Polygon.
     */
    [[nodiscard]] bool operator==(const RPolygon& poly) const { return vertices_ == poly.vertices_; }

    /**
     * Compare two polygons.
     * @param poly               Polygon.
     */
    [[nodiscard]] bool operator!=(const RPolygon& poly) const { return vertices_ != poly.vertices_; }

private:

    /**
     * Remove the closing, duplicate and aligned vertices.
     */
    void simplify();

    //-----------------------------------------------------
    std::vector<RPoint> vertices_;          ///< Vertices.
};


//------------------------------------------------------------------------------
/**
 * Get the name of an orientation ("normal", "mirror_x", "rota90", etc.).
 * @param ori                Orientation.
 */
[[nodiscard]] std::string to_string(ROrientation ori);


//------------------------------------------------------------------------------
/**
 * Write a polygon.
 * @param os                     Stream.
 * @param poly                   Polygon.
 */
std::ostream& operator<<(std::ostream& os, const RPolygon& poly);


}  //------- End of namespace evocomb ----------------------------------------------
