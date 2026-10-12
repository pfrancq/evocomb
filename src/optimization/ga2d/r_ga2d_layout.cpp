/*

    EvoComb

    RGA2DLayout.cpp

    Layout and Grid of the 2D Placement - Implementation.

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
#include <limits>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "app/r_logger.hpp"
#include "ga/r_ga.hpp"
#include "ga2d/r_ga2d_layout.hpp"
#include "graph/r_graph.hpp"
#include "tools/r_exception.hpp"
using namespace evocomb;


//------------------------------------------------------------------------------
mLogModule("r.2dga");



//------------------------------------------------------------------------------
//
// class RGA2DGrid
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
RGA2DGrid::RGA2DGrid(const RCoord width, const RCoord height) :
    width_(width),
    height_(height)
{
    // Limit the memory used (about 400 MB)
    constexpr RCoord maxCells = 100'000'000;
    if ((width <= 0) || (height <= 0) || (width > maxCells / height))
        mThrowRException("Invalid size of the grid: ", width, "x", height);
    cells_.assign(static_cast<std::size_t>(width * height), 0);
}


//------------------------------------------------------------------------------
void RGA2DGrid::clear()
{
    std::fill(cells_.begin(), cells_.end(), 0);
}


//------------------------------------------------------------------------------
void RGA2DGrid::assign(const std::vector<RRect>& rects, const RPoint& pos, const std::size_t id)
{
    for (const auto& r : rects)
    {
        const auto rect = r.translated(pos);
        if ((rect.x1 < 0) || (rect.y1 < 0) || (rect.x2 > width_) || (rect.y2 > height_))
            mThrowRException("Rectangle ", rect, " of object ", id, " is outside the grid");
        for (auto y = rect.y1; y < rect.y2; ++y)
        {
            auto* row = &cells_[static_cast<std::size_t>(y * width_)];
            std::fill(row + rect.x1, row + rect.x2, static_cast<std::uint32_t>(id + 1));
        }
    }
}


//------------------------------------------------------------------------------
bool RGA2DGrid::isFree(const RRect& rect) const
{
    for (auto y = rect.y1; y < rect.y2; ++y)
    {
        const auto* row = &cells_[static_cast<std::size_t>(y * width_)];
        if (std::any_of(row + rect.x1, row + rect.x2, [](const std::uint32_t cell) { return cell != 0; }))
            return false;
    }
    return true;
}


//------------------------------------------------------------------------------
bool RGA2DGrid::fits(const RGA2DShape& shape, const RPoint& pos) const
{
    if ((pos.x < 0) || (pos.y < 0) || (pos.x + shape.width > width_) || (pos.y + shape.height > height_))
        return false;
    return std::all_of(shape.rects->begin(), shape.rects->end(), [&](const RRect& rect) { return isFree(rect.translated(pos)); });
}


//------------------------------------------------------------------------------
bool RGA2DGrid::canMove(const RGA2DShape& shape, const RPoint& pos, const RCoord dx, const RCoord dy) const
{
    const RPoint dest(pos.x + dx, pos.y + dy);
    if ((dest.x < 0) || (dest.y < 0) || (dest.x + shape.width > width_) || (dest.y + shape.height > height_))
        return false;

    // Only the squares entered by each rectangle are tested
    for (const auto& r : *shape.rects)
    {
        const auto rect = r.translated(pos);
        RRect entered;
        if (dx < 0)
            entered = RRect(rect.x1 - 1, rect.y1, rect.x1, rect.y2);
        else if (dx > 0)
            entered = RRect(rect.x2, rect.y1, rect.x2 + 1, rect.y2);
        else if (dy < 0)
            entered = RRect(rect.x1, rect.y1 - 1, rect.x2, rect.y1);
        else
            entered = RRect(rect.x1, rect.y2, rect.x2, rect.y2 + 1);
        if (!isFree(entered))
            return false;
    }
    return true;
}


//------------------------------------------------------------------------------
bool RGA2DGrid::isFree(const RCoord x, const RCoord y) const
{
    if ((x < 0) || (y < 0) || (x >= width_) || (y >= height_))
        return false;
    return cells_[static_cast<std::size_t>(y * width_ + x)] == 0;
}


//------------------------------------------------------------------------------
std::size_t RGA2DGrid::objAt(const RCoord x, const RCoord y) const
{
    if ((x < 0) || (y < 0) || (x >= width_) || (y >= height_))
        return cNoRef;
    const auto cell = cells_[static_cast<std::size_t>(y * width_ + x)];
    return cell ? static_cast<std::size_t>(cell - 1) : cNoRef;
}



//------------------------------------------------------------------------------
//
// class RGA2DLayout
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
void RGA2DLayout::initLayout(const RGA2DProblem& problem)
{
    problem_ = &problem;
    placements_.assign(problem.nbObjs(), Placement());
    nbPlaced_ = 0;
}


//------------------------------------------------------------------------------
void RGA2DLayout::clearLayout()
{
    std::fill(placements_.begin(), placements_.end(), Placement());
    nbPlaced_ = 0;
}


//------------------------------------------------------------------------------
void RGA2DLayout::place(const std::size_t id, const std::size_t config, const RPoint& pos, const std::size_t order)
{
    auto& p = placements_[id];
    if (p.config == cNoRef)
        ++nbPlaced_;
    p.config = config;
    p.pos = pos;
    if (order != cNoRef)
        p.order = order;
}


//------------------------------------------------------------------------------
void RGA2DLayout::unplace(const std::size_t id)
{
    auto& p = placements_[id];
    if (p.config == cNoRef)
        return;
    --nbPlaced_;
    p = Placement();
}


//------------------------------------------------------------------------------
RRect RGA2DLayout::rect(const std::size_t id) const
{
    const auto& cfg = config(id);
    const auto& pos = placements_[id].pos;
    return {pos.x, pos.y, pos.x + cfg.width, pos.y + cfg.height};
}


//------------------------------------------------------------------------------
RRect RGA2DLayout::boundary() const
{
    RRect res;
    for (std::size_t id = 0; id < placements_.size(); ++id)
        if (isPlaced(id))
            res = res.united(rect(id));
    return res;
}


//------------------------------------------------------------------------------
bool RGA2DLayout::centers(const RGA2DNet::Item& item, std::vector<RPoint>& centers) const
{
    centers.clear();
    if (item.obj == cNoRef)
    {
        centers = problem_->boardCenters(item.connector);
        return !centers.empty();
    }
    if (!isPlaced(item.obj))
        return false;
    const auto& pos = placements_[item.obj].pos;
    const auto& pins = config(item.obj).centers[item.connector];
    if (pins.empty())
        centers.push_back(rect(item.obj).center());
    else
        for (const auto& pin : pins)
            centers.push_back(pin + pos);
    return true;
}


//------------------------------------------------------------------------------
double RGA2DLayout::netLength(const std::size_t net) const
{
    // Placed connectors
    const auto& items = problem_->nets()[net].items;
    std::vector<std::vector<RPoint>> nodes;
    std::vector<RPoint> tmp;
    for (const auto& item : items)
        if (centers(item, tmp))
            nodes.push_back(tmp);
    if (nodes.size() < 2)
        return 0.0;

    // Complete graph over the placed connectors: the weight of an edge is the smallest Manhattan distance between the
    // pins of the two connectors. The graph and the tree are working structures reused by each thread.
    thread_local RGraph graph;
    thread_local std::vector<std::size_t> tree;
    graph.clear(nodes.size());
    for (std::size_t a = 0; a < nodes.size(); ++a)
        for (std::size_t b = a + 1; b < nodes.size(); ++b)
        {
            auto min = std::numeric_limits<RCoord>::max();
            for (const auto& p1 : nodes[a])
                for (const auto& p2 : nodes[b])
                    min = std::min(min, p1.manhattan(p2));
            graph.addEdge(a, b, static_cast<double>(min));
        }
    return graph.minSpanningTree(tree);
}


//------------------------------------------------------------------------------
double RGA2DLayout::objLength(const std::size_t id, const bool weighted) const
{
    double sum = 0.0;
    for (const auto net : problem_->objNets(id))
        sum += netLength(net) * (weighted ? problem_->nets()[net].weight : 1.0);
    return sum;
}


//------------------------------------------------------------------------------
double RGA2DLayout::totalLength(const bool weighted) const
{
    double sum = 0.0;
    for (std::size_t net = 0; net < problem_->nets().size(); ++net)
        sum += netLength(net) * (weighted ? problem_->nets()[net].weight : 1.0);
    return sum;
}


//------------------------------------------------------------------------------
double RGA2DLayout::connectedWeight(const std::size_t id) const
{
    double weight = 0.0;
    for (const auto net : problem_->objNets(id))
    {
        const auto& items = problem_->nets()[net].items;
        const auto connected = std::any_of(items.begin(), items.end(), [this, id](const RGA2DNet::Item& item) {
            return (item.obj == cNoRef) || ((item.obj != id) && isPlaced(item.obj));
        });
        if (connected)
            weight += problem_->nets()[net].weight;
    }
    return weight;
}


//------------------------------------------------------------------------------
void RGA2DLayout::verifyLayout(const bool complete) const
{
    const RRect limits(0, 0, problem_->width(), problem_->height());
    if (complete && (nbPlaced_ != placements_.size()))
        mThrowRGAException(placements_.size() - nbPlaced_, " object(s) not placed");
    std::size_t nb = 0;
    for (std::size_t id = 0; id < placements_.size(); ++id)
    {
        if (!isPlaced(id))
            continue;
        ++nb;
        if (placements_[id].config >= problem_->obj(id).configs().size())
            mThrowRGAException("Object '", problem_->obj(id).name(), "' has an invalid configuration");
        if (!limits.contains(rect(id)))
            mThrowRGAException("Object '", problem_->obj(id).name(), "' is outside the limits");
    }
    if (nb != nbPlaced_)
        mThrowRGAException(nb, " objects placed instead of ", nbPlaced_);

    // Overlaps (the bounding rectangles are compared first)
    for (std::size_t i = 0; i < placements_.size(); ++i)
    {
        if (!isPlaced(i))
            continue;
        for (std::size_t j = i + 1; j < placements_.size(); ++j)
        {
            if ((!isPlaced(j)) || (!rect(i).overlaps(rect(j))))
                continue;
            for (const auto& r1 : config(i).rects)
                for (const auto& r2 : config(j).rects)
                    if (r1.translated(placements_[i].pos).overlaps(r2.translated(placements_[j].pos)))
                        mThrowRGAException("Objects '", problem_->obj(i).name(), "' and '", problem_->obj(j).name(), "' overlap");
        }
    }
}
