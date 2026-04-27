/*

    EvoComb

    RRect.hpp

    Rectangle with integer coordinates - Header.

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
#include <ostream>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "geometry/r_point.hpp"


//------------------------------------------------------------------------------
namespace evocomb {
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
 * The RRect class provides a rectangle with integer coordinates. The rectangle is half-open: it covers [x1,x2[ x
 * [y1,y2[, so that its width is x2-x1 and two adjacent rectangles share a coordinate without overlapping:
 * @code
 * evocomb::RRect a(0, 0, 10, 5);         // 10 x 5
 * evocomb::RRect b(10, 0, 20, 5);        // Adjacent to a
 * a.overlaps(b);                     // false
 * @endcode
 * @short Rectangle.
 */
struct RRect
{
    RCoord x1 = 0;                          ///< Left coordinate (included).
    RCoord y1 = 0;                          ///< Bottom coordinate (included).
    RCoord x2 = 0;                          ///< Right coordinate (excluded).
    RCoord y2 = 0;                          ///< Top coordinate (excluded).

    /**
     * Construct an empty rectangle.
     */
    constexpr RRect() = default;

    /**
     * Construct a rectangle (the coordinates are ordered if necessary).
     * @param px1                First X coordinate.
     * @param py1                First Y coordinate.
     * @param px2                Second X coordinate.
     * @param py2                Second Y coordinate.
     */
    constexpr RRect(const RCoord px1, const RCoord py1, const RCoord px2, const RCoord py2) :
        x1(std::min(px1, px2)), y1(std::min(py1, py2)), x2(std::max(px1, px2)), y2(std::max(py1, py2))
    {
    }

    /**
     * Get the width.
     */
    [[nodiscard]] constexpr RCoord width() const { return x2 - x1; }

    /**
     * Get the height.
     */
    [[nodiscard]] constexpr RCoord height() const { return y2 - y1; }

    /**
     * Get the area.
     */
    [[nodiscard]] constexpr RCoord area() const { return width() * height(); }

    /**
     * Look if the rectangle is empty (null width or height).
     */
    [[nodiscard]] constexpr bool empty() const { return x2 <= x1 || y2 <= y1; }

    /**
     * Get the bottom-left corner.
     */
    [[nodiscard]] constexpr RPoint bottomLeft() const { return {x1, y1}; }

    /**
     * Get the center (rounded down, also for the negative coordinates).
     */
    [[nodiscard]] constexpr RPoint center() const { return {floorHalf(x1 + x2), floorHalf(y1 + y2)}; }

    /**
     * Look if two rectangles overlap (share at least one unit square).
     * @param rect               Rectangle.
     */
    [[nodiscard]] constexpr bool overlaps(const RRect& rect) const { return !empty() && !rect.empty() && x1 < rect.x2 && rect.x1 < x2 && y1 < rect.y2 && rect.y1 < y2; }

    /**
     * Look if a unit square (given by its bottom-left corner) is in the rectangle.
     * @param pt                 Point.
     */
    [[nodiscard]] constexpr bool contains(const RPoint& pt) const { return pt.x >= x1 && pt.x < x2 && pt.y >= y1 && pt.y < y2; }

    /**
     * Look if a rectangle is included in the current one.
     * @param rect               Rectangle.
     */
    [[nodiscard]] constexpr bool contains(const RRect& rect) const { return rect.x1 >= x1 && rect.x2 <= x2 && rect.y1 >= y1 && rect.y2 <= y2; }

    /**
     * Get the rectangle translated by a vector.
     * @param pt                 Vector.
     */
    [[nodiscard]] constexpr RRect translated(const RPoint& pt) const { return {x1 + pt.x, y1 + pt.y, x2 + pt.x, y2 + pt.y}; }

    /**
     * Get the smallest rectangle containing two rectangles (an empty rectangle is ignored).
     * @param rect               Rectangle.
     */
    [[nodiscard]] constexpr RRect united(const RRect& rect) const
    {
        if (empty())
            return rect;
        if (rect.empty())
            return *this;
        return {std::min(x1, rect.x1), std::min(y1, rect.y1), std::max(x2, rect.x2), std::max(y2, rect.y2)};
    }

    /**
     * Compare two rectangles.
     * @param rect               Rectangle.
     */
    [[nodiscard]] constexpr bool operator==(const RRect& rect) const { return x1 == rect.x1 && y1 == rect.y1 && x2 == rect.x2 && y2 == rect.y2; }

    /**
     * Compare two rectangles.
     * @param rect               Rectangle.
     */
    [[nodiscard]] constexpr bool operator!=(const RRect& rect) const { return !(*this == rect); }

private:

    /**
     * Divide by two, rounding down.
     * @param v                  Value.
     */
    [[nodiscard]] static constexpr RCoord floorHalf(const RCoord v) { return v >= 0 ? v / 2 : -((-v + 1) / 2); }
};


//------------------------------------------------------------------------------
/**
 * Write a rectangle ("[x1,y1;x2,y2[").
 * @param os                     Stream.
 * @param rect                   Rectangle.
 */
inline std::ostream& operator<<(std::ostream& os, const RRect& rect)
{
    return os << '[' << rect.x1 << ',' << rect.y1 << ';' << rect.x2 << ',' << rect.y2 << '[';
}


}  //------- End of namespace evocomb ----------------------------------------------
