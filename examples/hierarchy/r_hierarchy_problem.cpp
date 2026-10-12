/*

    EvoComb - Hierarchy example

    RHierarchyProblem.cpp

    Hierarchy Problem - Implementation.

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
#include <fstream>
#include <sstream>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "r_hierarchy_problem.hpp"
#include "tools/r_exception.hpp"
using namespace evocomb;
using namespace evocomb::hierarchy;



//------------------------------------------------------------------------------
//
// class RHierarchyProblem
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
void RHierarchyProblem::read(const RJSON& json)
{
    objs_.clear();
    ptrs_.clear();
    labels_.clear();
    ids_.clear();

    const auto objs = json.find("objects");
    if ((objs == nullptr) || (!objs->isArray()))
        throw RException("The problem must contain an array 'objects'");
    for (std::size_t i = 0; i < objs->size(); ++i)
    {
        const auto& obj = (*objs)[i];
        if (!obj.isObject())
            throw RException("Object " + std::to_string(i) + " must be a JSON object");

        // Name (optional)
        std::string name = "Object " + std::to_string(i);
        if (const auto param = obj.find("name"); param != nullptr)
        {
            if (!param->isString())
                throw RException("The name of object " + std::to_string(i) + " must be a string");
            name = param->toString();
        }

        // Attributes
        const auto attrs = obj.find("attributes");
        if ((attrs == nullptr) || (!attrs->isArray()) || (attrs->size() == 0))
            throw RException("Object '" + name + "' must have a non-empty array 'attributes'");
        auto newObj = std::make_unique<RHGAObj>(i, name);
        for (std::size_t j = 0; j < attrs->size(); ++j)
        {
            const auto& attr = (*attrs)[j];
            if (!attr.isString())
                throw RException("The attributes of object '" + name + "' must be strings");
            const auto label = attr.toString();
            auto it = ids_.find(label);
            if (it == ids_.end())
            {
                it = ids_.emplace(label, labels_.size()).first;
                labels_.push_back(label);
            }
            newObj->addAttr(it->second);
        }
        ptrs_.push_back(newObj.get());
        objs_.push_back(std::move(newObj));
    }
    if (objs_.empty())
        throw RException("The problem contains no object");
}


//------------------------------------------------------------------------------
void RHierarchyProblem::load(const std::string& path)
{
    std::ifstream file(path);
    if (!file)
        throw RException("Cannot open '" + path + "'");
    std::stringstream str;
    str << file.rdbuf();
    read(RJSON::parse(str.str()));
}


//------------------------------------------------------------------------------
std::unique_ptr<RJSON> RHierarchyProblem::labels(const RHGAAttrList& attrs) const
{
    auto array = std::make_unique<RJSON>(RJSONType::Array);
    for (const auto attr : attrs)
        array->add(std::make_unique<RJSON>(labels_[attr]));
    return array;
}
