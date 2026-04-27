/*

    EvoComb

    RGA2DHeuristic.cpp

    Placement Heuristics of the 2D Placement Genetic Algorithm - Implementation.

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
#include <utility>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "ga2d/r_ga2d_heuristic.hpp"
#include "tools/r_concat.hpp"
using namespace evocomb;


//------------------------------------------------------------------------------
mLogModule("r.2dga");



//------------------------------------------------------------------------------
//
// class RGA2DBlock
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
RGA2DBlock RGA2DBlock::build(const RGA2DLayout& layout, const std::vector<std::size_t>& objs)
{
    RGA2DBlock block;
    RRect bound;
    for (const auto obj : objs)
        bound = bound.united(layout.rect(obj));
    const RPoint origin(bound.x1, bound.y1);
    for (const auto obj : objs)
    {
        const auto& p = layout.placement(obj);
        const auto pos = p.pos - origin;
        block.members.push_back({obj, p.config, pos});
        for (const auto& rect : layout.config(obj).rects)
            block.rects.push_back(rect.translated(pos));
    }
    block.width = bound.width();
    block.height = bound.height();
    return block;
}



//------------------------------------------------------------------------------
//
// class RGA2DHeuristic
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
RGA2DHeuristic::RGA2DHeuristic(RLogger& logger, std::string name, const RGA2DContext& context) :
    name_(std::move(name)),
    context_(context),
    prom_(logger, context.placementCriteria())
{
}


//------------------------------------------------------------------------------
void RGA2DHeuristic::run(RLogger& logger, RGA2DLayout& layout, RGA2DGrid& grid, const std::vector<RGA2DBlock>& blocks, RGARandom& random)
{
    const auto& problem = context_.problem();
    layout_ = &layout;
    grid_ = &grid;
    width_ = problem.width();
    height_ = problem.height();
    grid.clear();
    layout.clearLayout();
    result_ = RRect();
    nbPlaced_ = 0;

    // Items to place: the blocks and the objects not in a block
    items_.clear();
    std::vector<char> inBlock(problem.nbObjs(), 0);
    for (const auto& block : blocks)
    {
        items_.push_back({cNoRef, &block});
        for (const auto& member : block.members)
            inBlock[member.obj] = 1;
    }
    for (std::size_t obj = 0; obj < problem.nbObjs(); ++obj)
        if (!inBlock[obj])
            items_.push_back({obj, nullptr});
    std::shuffle(items_.begin(), items_.end(), random);

    // Place them
    init();
    while (nbPlaced_ < items_.size())
    {
        selectNext();
        placeCurrent(logger);
    }
}


//------------------------------------------------------------------------------
double RGA2DHeuristic::weight(const Item& item, const bool connected) const
{
    const auto objWeight = [&](const std::size_t obj) {
        return connected ? layout_->connectedWeight(obj) : context_.problem().connectorWeights(obj);
    };
    if (item.block == nullptr)
        return objWeight(item.obj);
    double sum = 0.0;
    for (const auto& member : item.block->members)
        sum += objWeight(member.obj);
    return sum;
}


//------------------------------------------------------------------------------
void RGA2DHeuristic::selectNext()
{
    // The first item is random, or the one having the highest weight of nets
    const auto connected = (nbPlaced_ > 0);
    if ((!connected) && context_.firstRandom())
        return;

    // Item with the highest weight (the first one in case of equality)
    auto best = nbPlaced_;
    auto bestWeight = weight(items_[best], connected);
    for (auto i = nbPlaced_ + 1; i < items_.size(); ++i)
        if (const auto w = weight(items_[i], connected); w > bestWeight)
        {
            best = i;
            bestWeight = w;
        }
    std::swap(items_[nbPlaced_], items_[best]);
}


//------------------------------------------------------------------------------
RGA2DShape RGA2DHeuristic::shape(const Item& item, const std::size_t config) const
{
    if (item.block != nullptr)
        return item.block->shape();
    const auto& cfg = context_.problem().obj(item.obj).configs()[config];
    return {&cfg.rects, cfg.width, cfg.height};
}


//------------------------------------------------------------------------------
void RGA2DHeuristic::place(const Item& item, const std::size_t config, const RPoint& pos, const bool grid)
{
    const auto& problem = context_.problem();
    if (item.block == nullptr)
    {
        layout_->place(item.obj, config, pos, nbPlaced_);
        if (grid)
            grid_->assign(problem.obj(item.obj).configs()[config].rects, pos, item.obj);
        return;
    }
    for (const auto& member : item.block->members)
    {
        layout_->place(member.obj, member.config, pos + member.pos, nbPlaced_);
        if (grid)
            grid_->assign(problem.obj(member.obj).configs()[member.config].rects, pos + member.pos, member.obj);
    }
}


//------------------------------------------------------------------------------
void RGA2DHeuristic::unplace(const Item& item)
{
    if (item.block == nullptr)
        layout_->unplace(item.obj);
    else
        for (const auto& member : item.block->members)
            layout_->unplace(member.obj);
}


//------------------------------------------------------------------------------
void RGA2DHeuristic::addPosition(const RPoint& pos)
{
    if (!grid_->fits(curShape_, pos))
        return;
    const auto same = std::any_of(candidates_.begin(), candidates_.end(), [&](const Candidate& c) {
        return (c.config == curConfig_) && (c.pos == pos);
    });
    if (same)
        return;

    // Area of the bounding rectangle of the placement with the item
    const RRect rect(pos.x, pos.y, pos.x + curShape_.width, pos.y + curShape_.height);
    const auto area = static_cast<double>(result_.united(rect).area());

    // Length of the nets of the item if it is placed at this position
    const auto& item = items_[nbPlaced_];
    const auto& problem = context_.problem();
    nets_.clear();
    const auto addNets = [&](const std::size_t obj) {
        for (const auto net : problem.objNets(obj))
            if (std::find(nets_.begin(), nets_.end(), net) == nets_.end())
                nets_.push_back(net);
    };
    if (item.block == nullptr)
        addNets(item.obj);
    else
        for (const auto& member : item.block->members)
            addNets(member.obj);
    place(item, curConfig_, pos, false);
    double dist = 0.0;
    for (const auto net : nets_)
        dist += layout_->netLength(net) * (context_.weightedDistances() ? problem.nets()[net].weight : 1.0);
    unplace(item);

    candidates_.push_back({curConfig_, pos, {area, dist}});
}


//------------------------------------------------------------------------------
void RGA2DHeuristic::pushBottomLeft(const RGA2DShape& shape, RPoint& pos) const
{
    auto change = true;
    while (change)
    {
        change = false;
        while (grid_->canMove(shape, pos, 0, -1))
        {
            --pos.y;
            change = true;
        }
        while (grid_->canMove(shape, pos, -1, 0))
        {
            --pos.x;
            change = true;
        }
    }
}


//------------------------------------------------------------------------------
void RGA2DHeuristic::pushCenter(const RGA2DShape& shape, RPoint& pos) const
{
    const RPoint center(width_ / 2, height_ / 2);
    const RCoord dx = (pos.x > center.x) ? -1 : 1;
    const RCoord dy = (pos.y > center.y) ? -1 : 1;
    auto change = true;
    while (change)
    {
        change = false;
        while ((pos.y != center.y) && grid_->canMove(shape, pos, 0, dy))
        {
            pos.y += dy;
            change = true;
        }
        while ((pos.x != center.x) && grid_->canMove(shape, pos, dx, 0))
        {
            pos.x += dx;
            change = true;
        }
    }
}


//------------------------------------------------------------------------------
void RGA2DHeuristic::placeCurrent(RLogger& logger)
{
    [[maybe_unused]] mLogModule("r.2dga");
    const auto& problem = context_.problem();
    const auto& item = items_[nbPlaced_];
    candidates_.clear();

    // Configurations to try
    std::vector<std::size_t> configs;
    if (item.block != nullptr)
        configs.push_back(cNoRef);
    else
    {
        const auto& cfgs = problem.obj(item.obj).configs();
        for (std::size_t c = 0; c < cfgs.size(); ++c)
            if ((cfgs[c].width <= width_) && (cfgs[c].height <= height_) && (context_.allOrientations() || configs.empty()))
                configs.push_back(c);
    }

    // Positions proposed by the heuristic
    for (const auto config : configs)
    {
        curConfig_ = config;
        curShape_ = shape(item, config);
        searchPositions(curShape_);
    }

    // If necessary, the first position that fits
    for (std::size_t i = 0; (i < configs.size()) && candidates_.empty(); ++i)
    {
        curConfig_ = configs[i];
        curShape_ = shape(item, configs[i]);
        auto found = false;
        for (RCoord y = 0; (y + curShape_.height <= height_) && (!found); ++y)
            for (RCoord x = 0; (x + curShape_.width <= width_) && (!found); ++x)
                if (RPoint pos(x, y); grid_->fits(curShape_, pos))
                {
                    pushBottomLeft(curShape_, pos);
                    addPosition(pos);
                    found = true;
                }
    }
    if (candidates_.empty())
    {
        if (item.block == nullptr)
            mThrowRGAException(name_, ": object '", problem.obj(item.obj).name(), "' cannot be placed");
        mThrowRGAException(name_, ": a block of ", item.block->members.size(), " objects cannot be placed");
    }

    // Choose the best position
    std::size_t best = 0;
    if (candidates_.size() > 1)
    {
        sols_.clear();
        for (const auto& candidate : candidates_)
            sols_.push_back(&candidate.criteria);
        prom_.compute(logger, sols_, fi_);
        for (std::size_t i = 1; i < fi_.size(); ++i)
            if (fi_[i] > fi_[best])
                best = i;
    }
    const auto& choice = candidates_[best];
    const auto s = shape(item, choice.config);
    place(item, choice.config, choice.pos, true);
    mLogDebug(logger, name_, ": ", (item.block ? "block" : problem.obj(item.obj).name()), " placed at ", choice.pos, " (", candidates_.size(), " positions)");
    postPlace(s, choice.pos);
    result_ = result_.united(RRect(choice.pos.x, choice.pos.y, choice.pos.x + s.width, choice.pos.y + s.height));
    ++nbPlaced_;
}



//------------------------------------------------------------------------------
//
// class RGA2DBottomLeftHeuristic
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
RGA2DBottomLeftHeuristic::RGA2DBottomLeftHeuristic(RLogger& logger, const RGA2DContext& context) :
    RGA2DHeuristic(logger, "bottom_left", context)
{
}


//------------------------------------------------------------------------------
void RGA2DBottomLeftHeuristic::init()
{
    nextObj_ = RPoint(0, 0);
    nextLine_ = RPoint(0, 0);
}


//------------------------------------------------------------------------------
void RGA2DBottomLeftHeuristic::searchPositions(const RGA2DShape& shape)
{
    // Push the object from the current position in the column
    RPoint pos(nextLine_);
    if (grid().fits(shape, pos))
        pushBottomLeft(shape, pos);

    // Too high or too wide for the column -> new column
    if ((pos.y + shape.height > result_.y2) || (pos.x + shape.width > nextObj_.x))
    {
        pos = nextObj_;
        if (grid().fits(shape, pos))
            pushBottomLeft(shape, pos);
    }

    // Too wide for the limits -> new row of columns
    if (pos.x + shape.width > width_)
    {
        nextObj_ = RPoint(0, result_.y2);
        pos = nextObj_;
        if (grid().fits(shape, pos))
            pushBottomLeft(shape, pos);
    }
    addPosition(pos);
}


//------------------------------------------------------------------------------
void RGA2DBottomLeftHeuristic::postPlace(const RGA2DShape& shape, const RPoint& pos)
{
    nextLine_ = nextObj_;
    nextObj_.x = std::max(nextObj_.x, pos.x + shape.width);
    nextLine_.y += shape.height;
}



//------------------------------------------------------------------------------
//
// class RGA2DEdgeHeuristic
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
RGA2DEdgeHeuristic::RGA2DEdgeHeuristic(RLogger& logger, const RGA2DContext& context) :
    RGA2DHeuristic(logger, "edge", context)
{
}


//------------------------------------------------------------------------------
void RGA2DEdgeHeuristic::init()
{
    levels_.clear();
    nbLevels_ = 0;
    curLevel_ = 0;
    actual_ = RPoint(0, 0);
    max_ = RPoint(0, 0);
    last_ = RPoint(0, 0);
}


//------------------------------------------------------------------------------
RPoint& RGA2DEdgeHeuristic::level(const std::size_t idx)
{
    if (idx >= levels_.size())
        levels_.resize(idx + 1, RPoint(0, 0));
    return levels_[idx];
}


//------------------------------------------------------------------------------
void RGA2DEdgeHeuristic::searchPositions(const RGA2DShape& shape)
{
    // Grow in width if the placement is proportionally higher than wide
    if ((result_.x2 < width_) && (curLevel_ == nbLevels_))
    {
        const auto factorX = static_cast<double>(result_.x2) / static_cast<double>(width_);
        const auto factorY = static_cast<double>(result_.y2) / static_cast<double>(height_);
        if (factorX < factorY)
        {
            level(curLevel_++) = actual_;
            if (curLevel_ >= nbLevels_)
                nbLevels_ = curLevel_;
            actual_ = RPoint(result_.x2, 0);
            max_.x = std::min(result_.x2 + shape.width, width_);
            curLevel_ = 0;
        }
    }

    // Push the object from the last position
    RPoint pos(last_);
    if (grid().fits(shape, pos))
        pushBottomLeft(shape, pos);
    if ((pos.y + shape.height > level(curLevel_).y) || (pos.x + shape.width > actual_.x))
    {
        pos = actual_;
        if (grid().fits(shape, pos))
            pushBottomLeft(shape, pos);
    }

    // Too wide -> next level
    while ((pos.x > 0) && (pos.x + shape.width > max_.x))
    {
        level(curLevel_++) = actual_;
        if (curLevel_ >= nbLevels_)
        {
            nbLevels_ = curLevel_;
            actual_ = RPoint(0, max_.y);
        }
        else
            actual_ = level(curLevel_);
        pos = actual_;
        if (grid().fits(shape, pos))
            pushBottomLeft(shape, pos);
    }

    // Too high -> no position
    if (pos.y + shape.height > height_)
        return;
    addPosition(pos);
}


//------------------------------------------------------------------------------
void RGA2DEdgeHeuristic::postPlace(const RGA2DShape& shape, const RPoint& pos)
{
    const auto x2 = pos.x + shape.width;
    const auto y2 = pos.y + shape.height;

    // Next positions
    last_ = actual_;
    if (x2 > actual_.x)
        actual_.x = x2;
    if ((pos.x == 0) && (x2 > max_.x))
        max_.x = x2;
    if (y2 > max_.y)
        max_.y = y2;
    last_.y += shape.height;

    // Update the lower levels
    for (std::size_t i = 0; i < curLevel_; ++i)
        if (level(i).x < max_.x)
            level(i).x = max_.x;

    // Skip the levels covered by the object
    const auto i = curLevel_ + 1;
    while ((i < nbLevels_) && (level(i).y < y2))
    {
        --nbLevels_;
        for (auto l = i; l < nbLevels_; ++l)
            level(l) = level(l + 1);
    }

    // First object reaching a new width
    if ((x2 > result_.x2) && (max_.x == 0))
        max_.x = x2;
}



//------------------------------------------------------------------------------
//
// class RGA2DCenterHeuristic
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
RGA2DCenterHeuristic::RGA2DCenterHeuristic(RLogger& logger, const RGA2DContext& context) :
    RGA2DHeuristic(logger, "center", context)
{
}


//------------------------------------------------------------------------------
std::vector<RPoint> RGA2DCenterHeuristic::corners(const std::vector<RRect>& rects)
{
    // Compressed coordinates
    std::vector<RCoord> xs;
    std::vector<RCoord> ys;
    for (const auto& r : rects)
    {
        xs.push_back(r.x1);
        xs.push_back(r.x2);
        ys.push_back(r.y1);
        ys.push_back(r.y2);
    }
    std::sort(xs.begin(), xs.end());
    xs.erase(std::unique(xs.begin(), xs.end()), xs.end());
    std::sort(ys.begin(), ys.end());
    ys.erase(std::unique(ys.begin(), ys.end()), ys.end());
    if (xs.size() < 2)
        return {};

    // Occupied cells of the compressed grid
    const auto nx = xs.size() - 1;
    const auto ny = ys.size() - 1;
    std::vector<char> occ(nx * ny, 0);
    const auto index = [](const std::vector<RCoord>& v, const RCoord c) {
        return static_cast<std::size_t>(std::lower_bound(v.begin(), v.end(), c) - v.begin());
    };
    for (const auto& r : rects)
        for (auto j = index(ys, r.y1); j < index(ys, r.y2); ++j)
            for (auto i = index(xs, r.x1); i < index(xs, r.x2); ++i)
                occ[j * nx + i] = 1;
    const auto isOcc = [&](const std::size_t i, const std::size_t j) {
        // Cell whose bottom-left corner is (xs[i],ys[j]); the indexes "before 0" wrap and are rejected as being outside
        return (i < nx) && (j < ny) && (occ[j * nx + i] != 0);
    };

    // A point is a corner if the four cells around it are not "aligned"
    std::vector<RPoint> res;
    for (std::size_t j = 0; j < ys.size(); ++j)
        for (std::size_t i = 0; i < xs.size(); ++i)
        {
            const auto a = isOcc(i - 1, j - 1);
            const auto b = isOcc(i, j - 1);
            const auto c = isOcc(i - 1, j);
            const auto d = isOcc(i, j);
            const auto nb = a + b + c + d;
            if ((nb == 1) || (nb == 3) || ((nb == 2) && (a == d)))
                res.emplace_back(xs[i], ys[j]);
        }
    return res;
}


//------------------------------------------------------------------------------
void RGA2DCenterHeuristic::init()
{
    placed_.clear();
    corners_.clear();
}


//------------------------------------------------------------------------------
void RGA2DCenterHeuristic::searchPositions(const RGA2DShape& shape)
{
    // First object -> center of the limits
    if (nbPlaced_ == 0)
    {
        addPosition(RPoint((width_ - shape.width) / 2, (height_ - shape.height) / 2));
        return;
    }

    // Positions touching a corner of the placed objects
    for (const auto& corner : corners_)
    {
        const RPoint tries[] = {
            {corner.x - shape.width, corner.y},
            {corner.x, corner.y - shape.height},
            {corner.x - shape.width, corner.y - shape.height},
            corner
        };
        for (auto pos : tries)
        {
            if (!grid().fits(shape, pos))
                continue;
            pushCenter(shape, pos);
            addPosition(pos);
        }
    }
}


//------------------------------------------------------------------------------
void RGA2DCenterHeuristic::postPlace(const RGA2DShape& shape, const RPoint& pos)
{
    for (const auto& rect : *shape.rects)
        placed_.push_back(rect.translated(pos));
    corners_ = corners(placed_);
}
