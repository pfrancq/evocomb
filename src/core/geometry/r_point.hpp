/*

    EvoComb

    RPoint.hpp

    Point with integer coordinates - Header.

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
#include <cstdint>
#include <cstdlib>
#include <ostream>


//------------------------------------------------------------------------------
namespace evocomb {
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
 * Type of the coordinates (the geometry works on an integer grid).
 */
using RCoord = std::int64_t;


//------------------------------------------------------------------------------
/**
 * The RPoint class provides a point with integer coordinates.
 * @short Point.
 */
struct RPoint
{
    RCoord x = 0;                           ///< X coordinate.
    RCoord y = 0;                           ///< Y coordinate.

    /**
     * Construct the point (0,0).
     */
    constexpr RPoint() = default;

    /**
     * Construct a point.
     * @param px                 X coordinate.
     * @param py                 Y coordinate.
     */
    constexpr RPoint(const RCoord px, const RCoord py) : x(px), y(py) {}

    /**
     * Compare two points.
     * @param pt                 Point.
     */
    [[nodiscard]] constexpr bool operator==(const RPoint& pt) const { return (x == pt.x) && (y == pt.y); }

    /**
     * Compare two points.
     * @param pt                 Point.
     */
    [[nodiscard]] constexpr bool operator!=(const RPoint& pt) const { return !(*this == pt); }

    /**
     * Order the points (by Y, then by X).
     * @param pt                 Point.
     */
    [[nodiscard]] constexpr bool operator<(const RPoint& pt) const { return (y < pt.y) || ((y == pt.y) && (x < pt.x)); }

    /**
     * Add a vector.
     * @param pt                 Vector.
     */
    constexpr RPoint& operator+=(const RPoint& pt) { x += pt.x; y += pt.y; return *this; }

    /**
     * Subtract a vector.
     * @param pt                 Vector.
     */
    constexpr RPoint& operator-=(const RPoint& pt) { x -= pt.x; y -= pt.y; return *this; }

    /**
     * Add two points.
     * @param pt                 Vector.
     */
    [[nodiscard]] constexpr RPoint operator+(const RPoint& pt) const { return {x + pt.x, y + pt.y}; }

    /**
     * Subtract two points.
     * @param pt                 Vector.
     */
    [[nodiscard]] constexpr RPoint operator-(const RPoint& pt) const { return {x - pt.x, y - pt.y}; }

    /**
     * Compute the Manhattan distance to another point.
     * @param pt                 Point.
     */
    [[nodiscard]] RCoord manhattan(const RPoint& pt) const { return std::llabs(x - pt.x) + std::llabs(y - pt.y); }
};


//------------------------------------------------------------------------------
/**
 * Write a point ("(x,y)").
 * @param os                     Stream.
 * @param pt                     Point.
 */
inline std::ostream& operator<<(std::ostream& os, const RPoint& pt)
{
    return os << '(' << pt.x << ',' << pt.y << ')';
}


}  //------- End of namespace evocomb ----------------------------------------------
