/*

    EvoComb - Hierarchy example

    RHierarchyHGA.hpp

    HGA for the Hierarchy Problem - Header.

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
#include <memory>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "app/r_logger.hpp"
#include "hga/r_hga_chromo.hpp"
#include "hga/r_hga_node.hpp"
#include "hga/r_hga_obj.hpp"
#include "hga/r_hga_population.hpp"
#include "r_hierarchy_problem.hpp"
#include "tools/r_json.hpp"


//------------------------------------------------------------------------------
namespace evocomb::hierarchy {
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
 * Node of the trees of the hierarchy problem.
 */
using RNodeHierarchy = RHGANode<RHGAObj>;


//------------------------------------------------------------------------------
/**
 * Thread-dependent data of the hierarchy problem.
 */
using RThreadDataHierarchy = RHGAThreadData<RNodeHierarchy, RHGAObj>;


//------------------------------------------------------------------------------
/**
 * The RChromoHierarchy class provides a chromosome of the hierarchy problem. Its fitness (criterion 0, to minimize) is
 * the average number of choices needed to reach an object from the top of the tree: a first choice is done among the
 * top nodes, and then, when a node is reached, one choice among its child nodes and its objects must be done. For
 * example, an object attached to a child of a top node, in a tree with 3 top nodes, the top node having 2 child nodes
 * and the child node 4 objects, needs 3 + 2 + 4 = 9 choices.
 * @short Hierarchy Chromosome.
 */
class RChromoHierarchy : public RHGAChromo<RNodeHierarchy, RHGAObj, RThreadDataHierarchy>
{
public:

    /**
     * Compute the fitness (see the class description).
     * @param logger             Logger.
     * @param data               Thread-dependent data.
     */
    void evaluate(RLogger& logger, RThreadDataHierarchy& data) override;

    /**
     * Compute the average number of choices to reach an object.
     */
    [[nodiscard]] double avgChoices() const;

    /**
     * Export the tree in JSON:
     * @code
     * [
     *   {
     *     "attributes": ["a", "b"],        // All the attributes of the node.
     *     "new_attributes": ["b"],         // The attributes that its parent has not.
     *     "objects": ["Object 1"],         // Names of the objects attached.
     *     "children": [ ... ]              // Child nodes.
     *   }
     * ]
     * @endcode
     * @param problem            Problem (for the labels of the attributes).
     * @return a JSON array containing the top nodes.
     */
    [[nodiscard]] std::unique_ptr<RJSON> toJSON(const RHierarchyProblem& problem) const;

private:

    /**
     * Add the number of choices to reach the objects of a branch.
     * @param id                 Identifier of the node.
     * @param choices            Number of choices to reach the node.
     * @param sum                Sum of the number of choices.
     */
    void addChoices(std::size_t id, double choices, double& sum) const;

    /**
     * Export a node in JSON.
     * @param id                 Identifier of the node.
     * @param problem            Problem.
     */
    [[nodiscard]] std::unique_ptr<RJSON> nodeToJSON(std::size_t id, const RHierarchyProblem& problem) const;
};


//------------------------------------------------------------------------------
/**
 * Population of the hierarchy problem (the fitness must be minimized, i.e. the parameter "maximize" of the GA must be
 * false).
 */
using RPopulationHierarchy = RHGAPopulation<RChromoHierarchy>;


}  //------- End of namespace evocomb::hierarchy -----------------------------------
