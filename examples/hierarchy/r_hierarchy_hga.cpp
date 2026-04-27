/*

    EvoComb - Hierarchy example

    RHierarchyHGA.cpp

    HGA for the Hierarchy Problem - Implementation.

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
#include "r_hierarchy_hga.hpp"
using namespace evocomb;
using namespace evocomb::hierarchy;



//------------------------------------------------------------------------------
//
// class RChromoHierarchy
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
void RChromoHierarchy::evaluate([[maybe_unused]] RLogger& logger, [[maybe_unused]] RThreadDataHierarchy& data)
{
    criterion(0, avgChoices());
}


//------------------------------------------------------------------------------
double RChromoHierarchy::avgChoices() const
{
    if (nbObjs() == 0)
        return 0.0;
    // The first choice is done among the top nodes
    const auto top = static_cast<double>(topNodes().size());
    double sum = 0.0;
    for (const auto id : topNodes())
        addChoices(id, top, sum);
    return sum / static_cast<double>(nbObjs());
}


//------------------------------------------------------------------------------
void RChromoHierarchy::addChoices(const std::size_t id, double choices, double& sum) const
{
    const auto& node = this->node(id);
    choices += static_cast<double>(node.nbNodes() + node.nbObjs());
    sum += static_cast<double>(node.nbObjs()) * choices;
    for (const auto child : node.children())
        addChoices(child, choices, sum);
}


//------------------------------------------------------------------------------
std::unique_ptr<RJSON> RChromoHierarchy::toJSON(const RHierarchyProblem& problem) const
{
    auto array = std::make_unique<RJSON>(RJSONType::Array);
    for (const auto id : topNodes())
        array->add(nodeToJSON(id, problem));
    return array;
}


//------------------------------------------------------------------------------
std::unique_ptr<RJSON> RChromoHierarchy::nodeToJSON(const std::size_t id, const RHierarchyProblem& problem) const
{
    const auto& node = this->node(id);
    auto json = std::make_unique<RJSON>(RJSONType::Object);
    json->add("attributes", problem.labels(node.attrs()));
    const auto& newAttrs = (node.parent() == cNoRef) ? node.attrs() : RHGAAttrList::diff(node.attrs(), this->node(node.parent()).attrs());
    json->add("new_attributes", problem.labels(newAttrs));
    auto objs = std::make_unique<RJSON>(RJSONType::Array);
    for (const auto obj : node.objs())
        objs->add(std::make_unique<RJSON>(obj->name()));
    json->add("objects", std::move(objs));
    auto children = std::make_unique<RJSON>(RJSONType::Array);
    for (const auto child : node.children())
        children->add(nodeToJSON(child, problem));
    json->add("children", std::move(children));
    return json;
}
