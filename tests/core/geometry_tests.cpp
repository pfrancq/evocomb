/*

    EvoComb

    RGeometryTests.cpp

    Tests of the geometry (points, rectangles and polygons).

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


//------------------------------------------------------------------------------
// include files for EvoComb
#include "r_google_test.hpp"
#include "geometry/r_polygon.hpp"
#include "tools/r_exception.hpp"
using namespace evocomb;


//------------------------------------------------------------------------------
mLogModule("r.unit_tests");


//------------------------------------------------------------------------------
class RGeometryTests : public RGoogleTest
{
};


//------------------------------------------------------------------------------
TEST_F(RGeometryTests, rect)
{
    const RRect a(0, 0, 10, 5);
    const RRect b(10, 0, 20, 5);
    EXPECT_EQ(a.width(), 10);
    EXPECT_EQ(a.area(), 50);
    EXPECT_FALSE(a.overlaps(b));                        // Adjacent
    EXPECT_TRUE(a.overlaps(RRect(9, 4, 12, 8)));
    EXPECT_TRUE(a.contains(RPoint(9, 4)));
    EXPECT_FALSE(a.contains(RPoint(10, 4)));
    EXPECT_EQ(a.united(b), RRect(0, 0, 20, 5));
    EXPECT_EQ(RRect(5, 6, 1, 2), RRect(1, 2, 5, 6));    // Ordered coordinates
    EXPECT_EQ(a.translated({1, 2}), RRect(1, 2, 11, 7));
    EXPECT_EQ(RPoint(1, 2).manhattan({4, -2}), 7);
    EXPECT_EQ(RRect(3, 3, 5, 5).center(), RPoint(4, 4));
    EXPECT_EQ(RRect(-1, -3, 0, 0).center(), RPoint(-1, -2));     // Rounded down
}


//------------------------------------------------------------------------------
TEST_F(RGeometryTests, polygon)
{
    // L-shaped polygon (with a closing vertex and an aligned one)
    const RPolygon l({{0, 0}, {5, 0}, {10, 0}, {10, 4}, {4, 4}, {4, 10}, {0, 10}, {0, 0}});
    EXPECT_EQ(l.size(), 6u);
    EXPECT_TRUE(l.isRectilinear());
    EXPECT_FALSE(l.isRect());
    EXPECT_EQ(l.boundary(), RRect(0, 0, 10, 10));
    EXPECT_EQ(l.area(), 10 * 4 + 4 * 6);

    // Decomposition: disjoint rectangles covering the same area
    const auto rects = l.rects();
    ASSERT_EQ(rects.size(), 2u);
    EXPECT_EQ(rects[0], RRect(0, 0, 10, 4));
    EXPECT_EQ(rects[1], RRect(0, 4, 4, 10));

    // U-shaped polygon
    const RPolygon u({{0, 0}, {9, 0}, {9, 9}, {6, 9}, {6, 3}, {3, 3}, {3, 9}, {0, 9}});
    const auto parts = u.rects();
    RCoord area = 0;
    for (std::size_t i = 0; i < parts.size(); ++i)
    {
        area += parts[i].area();
        for (std::size_t j = i + 1; j < parts.size(); ++j)
            EXPECT_FALSE(parts[i].overlaps(parts[j]));
    }
    EXPECT_EQ(area, u.area());
    EXPECT_EQ(parts.size(), 3u);

    // Invalid polygons
    EXPECT_FALSE(RPolygon({{0, 0}, {5, 5}, {0, 5}}).isRectilinear());
    EXPECT_THROW(static_cast<void>(RPolygon({{0, 0}, {5, 5}, {0, 5}}).rects()), RException);
}


//------------------------------------------------------------------------------
TEST_F(RGeometryTests, orientation)
{
    // A 10 x 4 rectangle with a pin near its bottom-left corner
    const RPolygon rect(RRect(0, 0, 10, 4));
    const RRect pin(1, 1, 2, 2);
    for (const auto ori : cOrientations)
    {
        RPoint offset;
        const auto poly = rect.transformed(ori, offset);
        const auto bound = poly.boundary();
        EXPECT_EQ(bound.x1, 0);
        EXPECT_EQ(bound.y1, 0);
        EXPECT_EQ(poly.area(), 40);
        const auto rotated = static_cast<int>(ori) >= static_cast<int>(ROrientation::Rota90);
        EXPECT_EQ(bound.width(), rotated ? 4 : 10);

        // The pin stays inside the transformed rectangle
        const auto p = RPolygon::transform(pin, ori).translated(offset);
        EXPECT_EQ(p.area(), 1);
        EXPECT_TRUE(bound.contains(p));
    }

    // Some explicit transformations
    RPoint offset;
    EXPECT_EQ(RPolygon::transform(pin, ROrientation::MirrorY).translated(offset = RPoint(10, 0)), RRect(8, 1, 9, 2));
    static_cast<void>(rect.transformed(ROrientation::Rota90, offset));
    EXPECT_EQ(RPolygon::transform(pin, ROrientation::Rota90).translated(offset), RRect(2, 1, 3, 2));
    EXPECT_EQ(to_string(ROrientation::Rota90MirrorX), "rota90_mirror_x");
}
