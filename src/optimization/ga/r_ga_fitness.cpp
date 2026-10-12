/*

    EvoComb

    RGAFitness.cpp

    Fitness strategies of the Genetic Algorithm - Implementation.

    Copyright 2026 by Pascal Francq (pascal@francq.info).

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
// include files for EvoComb
#include "r_ga_fitness.hpp"
#include "r_ga.hpp"
using namespace evocomb;


//------------------------------------------------------------------------------
mLogModule("r.ga");



//------------------------------------------------------------------------------
//
// class RGAFitness
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
std::unique_ptr<RGAFitness> RGAFitness::create(RLogger& logger, const RJSON* config)
{
    if (config == nullptr)
        return std::make_unique<RGADirectFitness>();
    if (!config->isObject())
        mThrowRGAException("'fitness' must be a JSON object");

    std::string type = "direct";
    if (const auto param = config->find("type"); param != nullptr)
    {
        if (!param->isString())
            mThrowRGAException("'fitness/type' must be a string");
        type = param->toString();
    }

    if (type == "direct")
        return std::make_unique<RGADirectFitness>();
    if (type == "promethee")
    {
        const auto criteria = config->find("criteria");
        if (criteria == nullptr)
            mThrowRGAException("'fitness/criteria' is mandatory for the PROMETHEE fitness");
        return std::make_unique<RGAPromFitness>(logger, *criteria);
    }
    mThrowRGAException("Unknown fitness type '", type, "' (\"direct\" or \"promethee\" expected)");
}



//------------------------------------------------------------------------------
//
// class RGADirectFitness
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
void RGADirectFitness::compute([[maybe_unused]] RLogger& logger, const std::vector<const std::vector<double>*>& criteria, std::vector<double>& fitness)
{
    fitness.resize(criteria.size());
    for (std::size_t i = 0; i < criteria.size(); ++i)
    {
        if (criteria[i]->empty())
            mThrowRGAException("Solution ", i, " has no criterion (RGAChromo::evaluate must set criterion 0)");
        fitness[i] = criteria[i]->front();
    }
}



//------------------------------------------------------------------------------
//
// class RGAPromFitness
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
RGAPromFitness::RGAPromFitness(RLogger& logger, const RJSON& criteria):
    kernel_(logger, "GA")
{
    kernel_.addCriteria(criteria);
    if (kernel_.nbCriteria() == 0)
        mThrowRGAException("The PROMETHEE fitness needs at least one criterion");
}


//------------------------------------------------------------------------------
void RGAPromFitness::compute(RLogger& logger, const std::vector<const std::vector<double>*>& criteria, std::vector<double>& fitness)
{
    mLogBlock(logger, "RGAPromFitness::compute");

    // Adapt the number of solutions if necessary
    if (kernel_.nbSolutions() != criteria.size())
    {
        kernel_.clearSols();
        std::vector<std::string> names;
        names.reserve(criteria.size());
        for (std::size_t i = 0; i < criteria.size(); ++i)
            names.push_back(std::to_string(i));
        kernel_.createSolutions(names);
    }

    // Assign the values and compute PROMETHEE II
    for (std::size_t i = 0; i < criteria.size(); ++i)
    {
        if (criteria[i]->size() != kernel_.nbCriteria())
            mThrowRGAException("Solution ", i, " has ", criteria[i]->size(), " criteria values instead of ", kernel_.nbCriteria());
        kernel_.assign(i, *criteria[i]);
    }
    kernel_.computePrometheeII();

    // The fitness is the Fi
    fitness.resize(criteria.size());
    for (const auto sol : kernel_.solutions())
        fitness[sol->index()] = sol->fi();
}
