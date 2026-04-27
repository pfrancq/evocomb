/*

    EvoComb

    RPolygon.cpp

    Rectilinear polygon with integer coordinates - Implementation.

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
#include <cstdlib>
#include <limits>
#include <utility>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "app/r_logger.hpp"
#include "geometry/r_polygon.hpp"
#include "tools/r_exception.hpp"
using namespace evocomb;


//------------------------------------------------------------------------------
mLogModule("r.core");



//------------------------------------------------------------------------------
//
// class RPolygon
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
RPolygon::RPolygon(std::vector<RPoint> vertices) :
    vertices_(std::move(vertices))
{
    simplify();
}


//------------------------------------------------------------------------------
RPolygon::RPolygon(const RRect& rect) :
    vertices_{{rect.x1, rect.y1}, {rect.x2, rect.y1}, {rect.x2, rect.y2}, {rect.x1, rect.y2}}
{
}


//------------------------------------------------------------------------------
void RPolygon::simplify()
{
    // Remove the consecutive duplicates (including the closing vertex)
    std::vector<RPoint> res;
    for (const auto& pt : vertices_)
        if (res.empty() || res.back() != pt)
            res.push_back(pt);
    while (res.size() > 1 && res.front() == res.back())
        res.pop_back();

    // Remove the vertices aligned with their neighbors
    auto changed = true;
    while (changed && res.size() > 2)
    {
        changed = false;
        for (std::size_t i = 0; i < res.size(); ++i)
        {
            const auto& prev = res[(i + res.size() - 1) % res.size()];
            const auto& cur = res[i];
            if (const auto& next = res[(i + 1) % res.size()]; (prev.x == cur.x && cur.x == next.x) || (prev.y == cur.y && cur.y == next.y))
            {
                res.erase(res.begin() + static_cast<std::ptrdiff_t>(i));
                changed = true;
                break;
            }
        }
    }
    vertices_ = std::move(res);
}


//------------------------------------------------------------------------------
bool RPolygon::isRectilinear() const
{
    if (vertices_.size() < 4)
        return false;
    for (std::size_t i = 0; i < vertices_.size(); ++i)
    {
        const auto& cur = vertices_[i];
        if (const auto& next = vertices_[(i + 1) % vertices_.size()]; cur.x != next.x && cur.y != next.y)
            return false;
    }
    return true;
}


//------------------------------------------------------------------------------
RRect RPolygon::boundary() const
{
    if (vertices_.empty())
        return {};
    auto x1 = std::numeric_limits<RCoord>::max();
    auto y1 = std::numeric_limits<RCoord>::max();
    auto x2 = std::numeric_limits<RCoord>::min();
    auto y2 = std::numeric_limits<RCoord>::min();
    for (const auto& pt : vertices_)
    {
        x1 = std::min(x1, pt.x);
        y1 = std::min(y1, pt.y);
        x2 = std::max(x2, pt.x);
        y2 = std::max(y2, pt.y);
    }
    return {x1, y1, x2, y2};
}


//------------------------------------------------------------------------------
RCoord RPolygon::area() const
{
    // Shoelace formula
    RCoord sum = 0;
    for (std::size_t i = 0; i < vertices_.size(); ++i)
    {
        const auto& cur = vertices_[i];
        const auto& next = vertices_[(i + 1) % vertices_.size()];
        sum += cur.x * next.y - next.x * cur.y;
    }
    return std::llabs(sum) / 2;
}


//------------------------------------------------------------------------------
std::vector<RRect> RPolygon::rects() const
{
    if (!isRectilinear())
        mThrowRException("The polygon is not rectilinear");

    // Horizontal slabs between two consecutive Y coordinates of the vertices
    std::vector<RCoord> ys;
    ys.reserve(vertices_.size());
    for (const auto& pt : vertices_)
        ys.push_back(pt.y);
    std::sort(ys.begin(), ys.end());
    ys.erase(std::unique(ys.begin(), ys.end()), ys.end());

    std::vector<RRect> res;
    std::vector<RRect> open;                // Rectangles that can be extended
    std::vector<RCoord> xs;
    for (std::size_t s = 0; s + 1 < ys.size(); ++s)
    {
        const auto ya = ys[s];
        const auto yb = ys[s + 1];

        // X coordinates of the vertical edges crossing the slab
        xs.clear();
        for (std::size_t i = 0; i < vertices_.size(); ++i)
        {
            const auto& cur = vertices_[i];
            if (const auto& next = vertices_[(i + 1) % vertices_.size()]; cur.x == next.x && std::min(cur.y, next.y) <= ya && std::max(cur.y, next.y) >= yb)
                xs.push_back(cur.x);
        }
        std::sort(xs.begin(), xs.end());
        if (xs.size() % 2)
            mThrowRException("The polygon is not simple");

        // Intervals inside the polygon: extend the open rectangles with the same interval, or create new ones
        std::vector<RRect> next;
        for (std::size_t i = 0; i < xs.size(); i += 2)
        {
            if (xs[i] == xs[i + 1])
                continue;
            if (const auto it = std::find_if(open.begin(), open.end(), [&](const RRect& r) { return r.x1 == xs[i] && r.x2 == xs[i + 1]; }); it != open.end())
            {
                RRect r = *it;
                r.y2 = yb;
                open.erase(it);
                next.push_back(r);
            }
            else
                next.emplace_back(xs[i], ya, xs[i + 1], yb);
        }
        res.insert(res.end(), open.begin(), open.end());
        open = std::move(next);
    }
    res.insert(res.end(), open.begin(), open.end());
    std::sort(res.begin(), res.end(), [](const RRect& a, const RRect& b) { return a.y1 < b.y1 || (a.y1 == b.y1 && a.x1 < b.x1); });
    return res;
}


//------------------------------------------------------------------------------
RPolygon RPolygon::translated(const RPoint& pt) const
{
    RPolygon res;
    res.vertices_.reserve(vertices_.size());
    for (const auto& v : vertices_)
        res.vertices_.push_back(v + pt);
    return res;
}


//------------------------------------------------------------------------------
RPolygon RPolygon::transformed(const ROrientation ori, RPoint& offset) const
{
    RPolygon res;
    res.vertices_.reserve(vertices_.size());
    for (const auto& v : vertices_)
        res.vertices_.push_back(transform(v, ori));
    const auto bound = res.boundary();
    offset = RPoint(-bound.x1, -bound.y1);
    for (auto& v : res.vertices_)
        v += offset;
    return res;
}


//------------------------------------------------------------------------------
RPoint RPolygon::transform(const RPoint& pt, const ROrientation ori)
{
    RPoint res(pt);
    const auto mirrorX = ori == ROrientation::MirrorX || ori == ROrientation::MirrorXY || ori == ROrientation::Rota90MirrorX || ori == ROrientation::Rota90MirrorXY;
    const auto mirrorY = ori == ROrientation::MirrorY || ori == ROrientation::MirrorXY || ori == ROrientation::Rota90MirrorY || ori == ROrientation::Rota90MirrorXY;
    const auto rotate = ori == ROrientation::Rota90 || ori == ROrientation::Rota90MirrorX || ori == ROrientation::Rota90MirrorY || ori == ROrientation::Rota90MirrorXY;
    if (mirrorX)
        res.y = -res.y;
    if (mirrorY)
        res.x = -res.x;
    if (rotate)
        res = RPoint(-res.y, res.x);
    return res;
}


//------------------------------------------------------------------------------
RRect RPolygon::transform(const RRect& rect, const ROrientation ori)
{
    const auto a = transform(RPoint(rect.x1, rect.y1), ori);
    const auto b = transform(RPoint(rect.x2, rect.y2), ori);
    return {a.x, a.y, b.x, b.y};
}


//------------------------------------------------------------------------------
std::string evocomb::to_string(const ROrientation ori)
{
    switch (ori)
    {
        case ROrientation::Normal: return "normal";
        case ROrientation::MirrorX: return "mirror_x";
        case ROrientation::MirrorY: return "mirror_y";
        case ROrientation::MirrorXY: return "mirror_xy";
        case ROrientation::Rota90: return "rota90";
        case ROrientation::Rota90MirrorX: return "rota90_mirror_x";
        case ROrientation::Rota90MirrorY: return "rota90_mirror_y";
        case ROrientation::Rota90MirrorXY: return "rota90_mirror_xy";
    }
    return "unknown";
}


//------------------------------------------------------------------------------
std::ostream& evocomb::operator<<(std::ostream& os, const RPolygon& poly)
{
    os << '{';
    for (std::size_t i = 0; i < poly.vertices().size(); ++i)
        os << (i ? " " : "") << poly.vertices()[i];
    return os << '}';
}
