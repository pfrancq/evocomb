/*

    EvoComb - Hierarchy example

    RHierarchyProblem.hpp

    Hierarchy Problem - Header.

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
#pragma once


//------------------------------------------------------------------------------
// include files for ANSI C/C++
#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <vector>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "hga/r_hga_obj.hpp"
#include "tools/r_json.hpp"


//------------------------------------------------------------------------------
namespace evocomb::hierarchy {
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
 * The RHierarchyProblem class provides a hierarchy problem: a set of objects described by attributes (for example
 * documents described by concepts) that must be organized in a tree whose nodes are sets of attributes. The problem is
 * read from a JSON document:
 * @code
 * {
 *   "objects": [
 *     {"name": "Neural networks", "attributes": ["networks", "neurons"]},
 *     {"name": "Metaheuristics", "attributes": ["optimization", "heuristics"]}
 *   ]
 * }
 * @endcode
 * Each attribute (a string) receives an identifier (see attrLabel).
 * @short Hierarchy Problem.
 */
class RHierarchyProblem
{
public:

    /**
     * Construct an empty problem.
     */
    RHierarchyProblem() = default;

    /**
     * Read the problem from a JSON document (see the class description).
     * @param json               JSON document.
     * @exception RException     The document is invalid.
     */
    void read(const RJSON& json);

    /**
     * Load the problem from a JSON file.
     * @param path               Path of the file.
     * @exception RException     The file cannot be read or is invalid.
     */
    void load(const std::string& path);

    /**
     * Get the objects.
     */
    [[nodiscard]] const std::vector<const RHGAObj*>& objs() const { return ptrs_; }

    /**
     * Get the number of different attributes.
     */
    [[nodiscard]] std::size_t nbAttrs() const { return labels_.size(); }

    /**
     * Get the label of an attribute.
     * @param id                 Identifier of the attribute.
     */
    [[nodiscard]] const std::string& attrLabel(const std::size_t id) const { return labels_[id]; }

    /**
     * Get the labels of a list of attributes.
     * @param attrs              Attributes.
     * @return a JSON array of strings.
     */
    [[nodiscard]] std::unique_ptr<RJSON> labels(const RHGAAttrList& attrs) const;

    //-----------------------------------------------------
    RHierarchyProblem(const RHierarchyProblem&) = delete;
    RHierarchyProblem& operator=(const RHierarchyProblem&) = delete;

private:

    //-----------------------------------------------------
    std::vector<std::unique_ptr<RHGAObj>> objs_;    ///< Objects.
    std::vector<const RHGAObj*> ptrs_;      ///< Pointers to the objects.
    std::vector<std::string> labels_;       ///< Labels of the attributes.
    std::map<std::string, std::size_t> ids_;    ///< Identifiers of the labels.
};


}  //------- End of namespace evocomb::hierarchy -----------------------------------
