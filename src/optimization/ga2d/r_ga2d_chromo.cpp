/*

    EvoComb

    RGA2DChromo.cpp

    Chromosome of the 2D Placement Genetic Algorithm - Implementation.

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
#include <numeric>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "ga2d/r_ga2d_chromo.hpp"
#include "tools/r_concat.hpp"
using namespace evocomb;


//------------------------------------------------------------------------------
mLogModule("r.2dga");



//------------------------------------------------------------------------------
//
// class RGA2DThreadData
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
RGA2DThreadData::RGA2DThreadData(RLogger& logger, const RGA2DContext& context) :
    context_(context),
    grid_(context.problem().width(), context.problem().height()),
    selection_(logger, context.selectionCriteria())
{
}


//------------------------------------------------------------------------------
void RGA2DThreadData::init(RLogger& logger)
{
    heuristic_ = createHeuristic(logger, context_.heuristic());
}


//------------------------------------------------------------------------------
std::unique_ptr<RGA2DHeuristic> RGA2DThreadData::createHeuristic(RLogger& logger, const std::string& name)
{
    if (name == "bottom_left")
        return std::make_unique<RGA2DBottomLeftHeuristic>(logger, context_);
    if (name == "edge")
        return std::make_unique<RGA2DEdgeHeuristic>(logger, context_);
    if (name == "center")
        return std::make_unique<RGA2DCenterHeuristic>(logger, context_);
    mThrowRGAException("Unknown heuristic '", name, "'");
}



//------------------------------------------------------------------------------
//
// class RGA2DChromo
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
void RGA2DChromo::setContext(const RGA2DContext& context)
{
    context_ = &context;
    initLayout(context.problem());
}


//------------------------------------------------------------------------------
void RGA2DChromo::randomConstruct(RLogger& logger, RGA2DThreadData& data, RGARandom& random)
{
    data.blocks().clear();
    data.heuristic().run(logger, *this, data.grid(), data.blocks(), random);
}


//------------------------------------------------------------------------------
std::vector<std::size_t> RGA2DChromo::selectObjs(RLogger& logger, RGA2DThreadData& data, RGARandom& random, const RGA2DLayout& parent) const
{
    auto& selected = data.selected();
    const auto nb = context_->problem().nbObjs();
    const auto weighted = context_->weightedDistances();

    // Maximal size of the group: a quarter of the placement of the parent
    const auto bound = parent.boundary();
    const auto maxWidth = bound.width() / 4;
    const auto maxHeight = bound.height() / 4;

    // Objects with connections, ranked with PROMETHEE, or, if none, the objects in a random order
    std::vector<std::size_t> ranking;
    std::vector<std::vector<double>> values;
    for (std::size_t obj = 0; obj < nb; ++obj)
    {
        if (selected[obj])
            continue;
        const auto weight = parent.connectedWeight(obj);
        const auto dist = parent.objLength(obj, weighted);
        if ((weight > 0.0) && (dist > 0.0))
        {
            ranking.push_back(obj);
            values.push_back({weight, dist});
        }
    }
    if (ranking.size() > 1)
    {
        std::vector<const std::vector<double>*> sols;
        for (const auto& v : values)
            sols.push_back(&v);
        std::vector<double> fi;
        data.selection().compute(logger, sols, fi);
        std::vector<std::size_t> idx(ranking.size());
        std::iota(idx.begin(), idx.end(), 0);
        std::stable_sort(idx.begin(), idx.end(), [&fi](const std::size_t a, const std::size_t b) { return fi[a] > fi[b]; });
        std::vector<std::size_t> sorted;
        for (const auto i : idx)
            sorted.push_back(ranking[i]);
        ranking = std::move(sorted);
    }
    else if (ranking.empty())
    {
        for (std::size_t obj = 0; obj < nb; ++obj)
            if (!selected[obj])
                ranking.push_back(obj);
        std::shuffle(ranking.begin(), ranking.end(), random);
    }
    if (ranking.empty())
        return {};

    // The best object, and the next one such that both fit in the maximal size
    std::vector<std::size_t> objs{ranking[0]};
    auto region = parent.rect(ranking[0]);
    for (std::size_t k = 1; k < ranking.size(); ++k)
    {
        const auto u = region.united(parent.rect(ranking[k]));
        if ((u.width() <= maxWidth) && (u.height() <= maxHeight))
        {
            objs.push_back(ranking[k]);
            region = u;
            break;
        }
    }

    // The objects inside the region
    for (std::size_t obj = 0; obj < nb; ++obj)
        if ((!selected[obj]) && (std::find(objs.begin(), objs.end(), obj) == objs.end()) && region.contains(parent.rect(obj)))
            objs.push_back(obj);
    for (const auto obj : objs)
        selected[obj] = 1;
    return objs;
}


//------------------------------------------------------------------------------
void RGA2DChromo::crossover(RLogger& logger, RGA2DThreadData& data, RGARandom& random, const RGA2DChromo& parent1, const RGA2DChromo& parent2)
{
    [[maybe_unused]] mLogModule("r.2dga");
    auto& blocks = data.blocks();
    blocks.clear();
    data.selected().assign(context_->problem().nbObjs(), 0);
    for (const auto* parent : {&parent1, &parent2})
    {
        const auto objs = selectObjs(logger, data, random, *parent);
        if (!objs.empty())
            blocks.push_back(RGA2DBlock::build(*parent, objs));
        mLogDebug(logger, "Crossover of chromosome ", id(), ": ", objs.size(), " object(s) kept from chromosome ", parent->id());
    }
    data.heuristic().run(logger, *this, data.grid(), blocks, random);
}


//------------------------------------------------------------------------------
void RGA2DChromo::mutation(RLogger& logger, RGA2DThreadData& data, RGARandom& random)
{
    randomConstruct(logger, data, random);
}


//------------------------------------------------------------------------------
void RGA2DChromo::evaluate([[maybe_unused]] RLogger& logger, [[maybe_unused]] RGA2DThreadData& data)
{
    const auto& problem = context_->problem();
    const auto limits = static_cast<double>(problem.width()) * static_cast<double>(problem.height());
    criterion(0, static_cast<double>(boundary().area()) / limits);
    criterion(1, totalLength(context_->weightedDistances()));
}


//------------------------------------------------------------------------------
void RGA2DChromo::verify([[maybe_unused]] RLogger& logger, [[maybe_unused]] RGA2DThreadData& data) const
{
    verifyLayout(true);
}
