/*

    EvoComb - Bin Packing example

    RChromoBP.cpp

    Grouping Genetic Algorithm for the Bin Packing Problem - Implementation.

    Copyright 1998-2026 by Pascal Francq (pascal@francq.info).

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/



//------------------------------------------------------------------------------
// include files for ANSI C/C++
#include <algorithm>
#include <limits>
#include <memory>
#include <utility>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "r_bp_gga.hpp"
#include "ga/r_ga.hpp"
using namespace evocomb;
using namespace evocomb::bp;


//------------------------------------------------------------------------------
mLogModule("r.bp");



//------------------------------------------------------------------------------
//
// class RGroupBP
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
void RGroupBP::verify() const
{
    std::size_t size = 0;
    for (const auto obj : objs())
        size += obj->size();
    if (size != size_)
        mThrowRGAException("Bin ", id(), ": size is ", size_, " instead of ", size);
    if (size_ > capacity_)
        mThrowRGAException("Bin ", id(), ": size (", size_, ") exceeds the capacity (", capacity_, ")");
}



//------------------------------------------------------------------------------
//
// class RThreadDataBP
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
std::unique_ptr<RThreadDataBP::Heuristic> RThreadDataBP::createHeuristic(RLogger& logger, const std::string& name)
{
    if (name == "first_fit_descending")
        return std::make_unique<FirstFitDescending>(context());
    return RGGAThreadData::createHeuristic(logger, name);
}



//------------------------------------------------------------------------------
//
// class RChromoBP
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
void RChromoBP::evaluate([[maybe_unused]] RLogger& logger, [[maybe_unused]] RThreadDataBP& data)
{
    double sum = 0.0;
    for (const auto id : used())
    {
        const auto ratio = group(id).ratio();
        sum += ratio * ratio;
    }
    criterion(0, (nbUsed() > 0) ? sum / static_cast<double>(nbUsed()) : 0.0);
}


//------------------------------------------------------------------------------
void RChromoBP::mutation(RLogger& logger, RThreadDataBP& data, RGARandom& random)
{
    mLogBlock(logger, "RChromoBP::mutation");

    // Release the least filled bin
    auto worstRatio = std::numeric_limits<double>::max();
    auto worst = cNoRef;
    for (const auto id : used())
    {
        const auto ratio = group(id).ratio();
        if (ratio > 1.0)
            mThrowRGAException("Bin ", id, ": the capacity is exceeded");
        if (ratio < worstRatio)
        {
            worstRatio = ratio;
            worst = id;
        }
    }
    if (worst != cNoRef)
        releaseGroup(worst);

    // Default mutation of the GGA
    RGGAChromo::mutation(logger, data, random);
}


//------------------------------------------------------------------------------
std::unique_ptr<RJSON> RChromoBP::toJSON() const
{
    auto bins = std::make_unique<RJSON>(RJSONType::Array);
    for (const auto id : used())
    {
        const auto& bin = group(id);
        auto json = std::make_unique<RJSON>(RJSONType::Object);
        json->add("size", std::make_unique<RJSON>(static_cast<long long>(bin.size())));
        auto objs = std::make_unique<RJSON>(RJSONType::Array);
        auto sizes = std::make_unique<RJSON>(RJSONType::Array);
        for (const auto obj : bin.objs())
        {
            objs->add(std::make_unique<RJSON>(static_cast<long long>(obj->id())));
            sizes->add(std::make_unique<RJSON>(static_cast<long long>(obj->size())));
        }
        json->add("objects", std::move(objs));
        json->add("sizes", std::move(sizes));
        bins->add(std::move(json));
    }
    return bins;
}


//------------------------------------------------------------------------------
RChromoBP::Heuristic& RChromoBP::repairHeuristic(RThreadDataBP& data)
{
    return data.ffd();
}


//------------------------------------------------------------------------------
void RChromoBP::localOptimize(RLogger& logger, RThreadDataBP& data, [[maybe_unused]] RGARandom& random)
{
    mLogBlock(logger, "RChromoBP::localOptimize");

    auto& free = data.tmpObjs();
    notAssigned(free);
    bool improved = true;
    while (improved && (!free.empty()))
    {
        // Treat the largest objects first
        std::stable_sort(free.begin(), free.end(), [](const RObjBP* a, const RObjBP* b) { return a->size() > b->size(); });
        improved = false;
        for (const auto id : used())
            if (optimizeBin(id, free, data))
            {
                improved = true;
                break;
            }
    }
}


//------------------------------------------------------------------------------
bool RChromoBP::optimizeBin(const std::size_t id, std::vector<const RObjBP*>& free, RThreadDataBP& data)
{
    const auto& bin = group(id);
    if (bin.size() == bin.capacity())
        return false;
    auto& del = data.delObjs();

    // Try to add two objects, then one object
    auto add1 = cNoRef;
    auto add2 = cNoRef;
    for (std::size_t i = 0; (i + 1 < free.size()) && (add1 == cNoRef); ++i)
        for (std::size_t j = i + 1; j < free.size(); ++j)
            if (canImprove(bin, free[i]->size() + free[j]->size(), del, data))
            {
                add1 = i;
                add2 = j;
                break;
            }
    for (std::size_t i = 0; (i < free.size()) && (add1 == cNoRef); ++i)
        if (canImprove(bin, free[i]->size(), del, data))
            add1 = i;
    if (add1 == cNoRef)
        return false;

    // Objects to add (removed from the free objects, the last one first)
    const auto obj1 = free[add1];
    const RObjBP* obj2 = nullptr;
    if (add2 != cNoRef)
    {
        obj2 = free[add2];
        free.erase(free.begin() + static_cast<std::ptrdiff_t>(add2));
    }
    free.erase(free.begin() + static_cast<std::ptrdiff_t>(add1));

    // Delete the objects from the bin, then insert the new ones
    for (const auto obj : del)
    {
        deleteObj(*obj);
        free.push_back(obj);
    }
    insertObj(id, *obj1);
    if (obj2 != nullptr)
        insertObj(id, *obj2);
    return true;
}


//------------------------------------------------------------------------------
bool RChromoBP::canImprove(const RGroupBP& bin, const std::size_t add, std::vector<const RObjBP*>& del, RThreadDataBP& data)
{
    del.clear();
    const auto size = bin.size();
    const auto capacity = bin.capacity();
    const auto newSize = size + add;
    if (newSize <= capacity)
        return true;

    // Objects of the bin by increasing size
    auto& objs = data.binObjs();
    objs = bin.objs();
    std::sort(objs.begin(), objs.end(), [](const RObjBP* a, const RObjBP* b) { return a->size() < b->size(); });
    const auto better = [size, capacity](const std::size_t s) { return (s <= capacity) && (s > size); };
    const auto nb = objs.size();

    // Delete one object
    for (std::size_t a = 0; a < nb; ++a)
        if (better(newSize - objs[a]->size()))
        {
            del = {objs[a]};
            return true;
        }

    // Delete two objects
    for (std::size_t a = 0; a < nb; ++a)
        for (std::size_t b = a + 1; b < nb; ++b)
            if (better(newSize - objs[a]->size() - objs[b]->size()))
            {
                del = {objs[a], objs[b]};
                return true;
            }

    // Delete three objects
    for (std::size_t a = 0; a < nb; ++a)
        for (std::size_t b = a + 1; b < nb; ++b)
            for (std::size_t c = b + 1; c < nb; ++c)
                if (better(newSize - objs[a]->size() - objs[b]->size() - objs[c]->size()))
                {
                    del = {objs[a], objs[b], objs[c]};
                    return true;
                }
    return false;
}
