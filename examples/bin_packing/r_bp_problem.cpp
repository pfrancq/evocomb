/*

    EvoComb - Bin Packing example

    RBPProblem.cpp

    Bin Packing Problem - Implementation.

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
#include <fstream>
#include <random>
#include <string>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "r_bp_problem.hpp"
#include "app/r_logger.hpp"
#include "tools/r_exception.hpp"
using namespace evocomb;
using namespace evocomb::bp;


//------------------------------------------------------------------------------
mLogModule("r.bp");



//------------------------------------------------------------------------------
//
// class RBPProblem
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
void RBPProblem::load(const std::filesystem::path& path)
{
    std::ifstream file(path);
    if (!file)
        mThrowRException("Cannot open '", path.string(), "'");

    clear();
    std::size_t nb = 0;
    if (!(file >> nb))
        mThrowRException("'", path.string(), "': invalid number of objects");
    objs_.reserve(nb);
    for (std::size_t i = 0; i < nb; ++i)
    {
        std::size_t size = 0;
        if (!(file >> size))
            mThrowRException("'", path.string(), "': invalid size for object ", i);
        objs_.emplace_back(i, size);
    }
    if (!(file >> capacity_) || (capacity_ == 0))
        mThrowRException("'", path.string(), "': invalid bin capacity");
    if (!(file >> minBins_))
        minBins_ = 0;
    for (const auto& obj : objs_)
        if (obj.size() > capacity_)
            mThrowRException("'", path.string(), "': object ", obj.id(), " is larger than the bin capacity");
}


//------------------------------------------------------------------------------
void RBPProblem::read(const RJSON& json)
{
    // Read a strictly positive integer
    const auto positive = [](const RJSON& obj, const std::string& key, const bool mandatory) -> std::size_t {
        const auto param = obj.find(key);
        if (param == nullptr)
        {
            if (mandatory)
                mThrowRException("'", key, "' is missing");
            return 0;
        }
        if ((!param->isInteger()) || (param->toInteger() <= 0))
            mThrowRException("'", key, "' must be a strictly positive integer");
        return static_cast<std::size_t>(param->toInteger());
    };

    if (!json.isObject())
        mThrowRException("The problem must be a JSON object");

    // Generated problem
    if (const auto gen = json.find("generate"); gen != nullptr)
    {
        if (!gen->isObject())
            mThrowRException("'generate' must be a JSON object");
        const auto seed = gen->find("seed") ? positive(*gen, "seed", true) : 1;
        generate(positive(*gen, "nb_bins", true), positive(*gen, "capacity", true), static_cast<std::uint32_t>(seed));
        return;
    }

    // Objects given
    clear();
    capacity_ = positive(json, "capacity", true);
    minBins_ = positive(json, "min_bins", false);
    const auto objs = json.find("objects");
    if ((objs == nullptr) || (!objs->isArray()) || (objs->size() == 0))
        mThrowRException("The problem must contain a non-empty array 'objects'");
    objs_.reserve(objs->size());
    for (std::size_t i = 0; i < objs->size(); ++i)
    {
        const auto& size = (*objs)[i];
        if ((!size.isInteger()) || (size.toInteger() <= 0))
            mThrowRException("The size of object ", i, " must be a strictly positive integer");
        if (static_cast<std::size_t>(size.toInteger()) > capacity_)
            mThrowRException("Object ", i, " is larger than the bin capacity");
        objs_.emplace_back(i, static_cast<std::size_t>(size.toInteger()));
    }
}


//------------------------------------------------------------------------------
void RBPProblem::generate(const std::size_t nbBins, const std::size_t capacity, const std::uint32_t seed)
{
    if ((nbBins == 0) || (capacity < 4))
        mThrowRException("Invalid parameters to generate a problem");
    clear();
    capacity_ = capacity;
    minBins_ = nbBins;

    // Split each bin into 2 to 4 objects
    std::mt19937 random(seed);
    std::vector<std::size_t> sizes;
    std::uniform_int_distribution<std::size_t> nbDist(2, 4);
    for (std::size_t bin = 0; bin < nbBins; ++bin)
    {
        const auto nb = nbDist(random);
        std::uniform_int_distribution<std::size_t> cutDist(1, capacity - 1);
        std::vector<std::size_t> cuts;
        while (cuts.size() < nb - 1)
        {
            const auto cut = cutDist(random);
            if (std::find(cuts.begin(), cuts.end(), cut) == cuts.end())
                cuts.push_back(cut);
        }
        std::sort(cuts.begin(), cuts.end());
        std::size_t prev = 0;
        for (const auto cut : cuts)
        {
            sizes.push_back(cut - prev);
            prev = cut;
        }
        sizes.push_back(capacity - prev);
    }

    // Mix the objects
    std::shuffle(sizes.begin(), sizes.end(), random);
    objs_.reserve(sizes.size());
    for (std::size_t i = 0; i < sizes.size(); ++i)
        objs_.emplace_back(i, sizes[i]);
}


//------------------------------------------------------------------------------
void RBPProblem::clear()
{
    objs_.clear();
    capacity_ = 0;
    minBins_ = 0;
}


//------------------------------------------------------------------------------
std::size_t RBPProblem::lowerBound() const
{
    if (capacity_ == 0)
        return 0;
    std::size_t total = 0;
    for (const auto& obj : objs_)
        total += obj.size();
    return (total + capacity_ - 1) / capacity_;
}


//------------------------------------------------------------------------------
std::vector<const RObjBP*> RBPProblem::objPtrs() const
{
    std::vector<const RObjBP*> ptrs;
    ptrs.reserve(objs_.size());
    for (const auto& obj : objs_)
        ptrs.push_back(&obj);
    return ptrs;
}
