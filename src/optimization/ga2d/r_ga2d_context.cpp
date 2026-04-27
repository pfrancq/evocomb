/*

    EvoComb

    RGA2DContext.cpp

    Context of the 2D Placement Genetic Algorithm - Implementation.

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
// include files for EvoComb
#include "app/r_logger.hpp"
#include "ga/r_ga.hpp"
#include "ga2d/r_ga2d_context.hpp"
using namespace evocomb;


//------------------------------------------------------------------------------
mLogModule("r.2dga");


//------------------------------------------------------------------------------
namespace {

/**
 * Read an optional boolean parameter.
 * @param obj                    JSON object.
 * @param key                    Key.
 * @param value                  Value to set.
 */
void readBool(const RJSON& obj, const std::string& key, bool& value)
{
    const auto param = obj.find(key);
    if (param == nullptr)
        return;
    if (!param->isBoolean())
        mThrowRGAException("'", key, "' must be a boolean");
    value = param->toBool();
}

/**
 * Read an optional array of criteria.
 * @param obj                    JSON object.
 * @param key                    Key.
 * @param value                  Value to set.
 */
void readCriteria(const RJSON& obj, const std::string& key, std::shared_ptr<const RJSON>& value)
{
    const auto param = obj.find(key);
    if (param == nullptr)
        return;
    if ((!param->isArray()) || (param->size() != 2))
        mThrowRGAException("'", key, "' must be an array of two criteria");
    value = std::make_shared<const RJSON>(*param);
}

}



//------------------------------------------------------------------------------
//
// class RGA2DContext
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
RGA2DContext::RGA2DContext(const RGA2DProblem& problem) :
    problem_(&problem),
    placementCriteria_(std::make_shared<const RJSON>(RJSON::parse(R"([
        {"name": "area", "type": "minimize"},
        {"name": "distance", "type": "minimize"}])"))),
    selectionCriteria_(std::make_shared<const RJSON>(RJSON::parse(R"([
        {"name": "weight", "type": "maximize"},
        {"name": "distance", "type": "minimize"}])")))
{
}


//------------------------------------------------------------------------------
void RGA2DContext::configure(const RJSON& config)
{
    const auto ga2d = config.find("ga2d");
    if (ga2d == nullptr)
        return;
    if (!ga2d->isObject())
        mThrowRGAException("'ga2d' must be a JSON object");
    if (const auto param = ga2d->find("heuristic"); param != nullptr)
    {
        if (!param->isString())
            mThrowRGAException("'heuristic' must be a string");
        heuristic_ = param->toString();
    }
    readBool(*ga2d, "all_orientations", allOrientations_);
    readBool(*ga2d, "weighted_distances", weightedDistances_);
    readBool(*ga2d, "first_random", firstRandom_);
    readCriteria(*ga2d, "placement_criteria", placementCriteria_);
    readCriteria(*ga2d, "selection_criteria", selectionCriteria_);
}
