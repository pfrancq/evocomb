/*

    EvoComb

    RGA2DContext.hpp

    Context of the 2D Placement Genetic Algorithm - Header.

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
#include <memory>
#include <string>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "ga2d/r_ga2d_problem.hpp"
#include "tools/r_json.hpp"


//------------------------------------------------------------------------------
namespace evocomb {
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
 * The RGA2DContext class provides the data shared by all the chromosomes of the 2D placement genetic algorithm: the
 * problem (see RGA2DProblem, which must be initialized) and the parameters, read from the "ga2d" object of the
 * configuration (all of them are optional):
 * @code
 * {
 *   "ga2d": {
 *     "heuristic": "bottom_left",        // "bottom_left", "edge" or "center".
 *     "all_orientations": false,         // Try all the orientations of the objects (or only the normal one)?
 *     "weighted_distances": false,       // Multiply the lengths of the nets by their weights?
 *     "first_random": true,              // Place first a random object (or the most connected one)?
 *     "placement_criteria": [...],       // PROMETHEE criteria choosing the position of an object.
 *     "selection_criteria": [...]        // PROMETHEE criteria choosing the objects kept by the crossover.
 *   }
 * }
 * @endcode
 * The heuristic places the objects one by one, and chooses the position of each object among several valid ones with
 * PROMETHEE II, on two criteria (in this order): the area of the bounding rectangle of the placed objects and the
 * length of the nets of the object (both minimized by default). The crossover chooses the objects to keep from a parent
 * with PROMETHEE II on two criteria (in this order): the weight of their nets (maximized by default) and their length
 * (minimized by default). The criteria are defined as for RPromKernel::addCriteria.
 * @short 2D Placement Context.
 */
class RGA2DContext
{
public:

    /**
     * Construct the context.
     * @param problem            Problem (initialized; it must live as long as the context).
     */
    explicit RGA2DContext(const RGA2DProblem& problem);

    /**
     * Destruct the context.
     */
    virtual ~RGA2DContext() = default;

    /**
     * Read the parameters from the "ga2d" object of a configuration (the parameters missing keep their values).
     * @param config             Configuration.
     * @exception RGAException   A parameter is invalid.
     */
    virtual void configure(const RJSON& config);

    /**
     * Get the problem.
     */
    [[nodiscard]] const RGA2DProblem& problem() const { return *problem_; }

    /**
     * Get the name of the heuristic.
     */
    [[nodiscard]] const std::string& heuristic() const { return heuristic_; }

    /**
     * Look if all the orientations of the objects are tried.
     */
    [[nodiscard]] bool allOrientations() const { return allOrientations_; }

    /**
     * Look if the lengths of the nets are multiplied by their weights.
     */
    [[nodiscard]] bool weightedDistances() const { return weightedDistances_; }

    /**
     * Look if the first object placed is chosen randomly (or if it is the most connected one).
     */
    [[nodiscard]] bool firstRandom() const { return firstRandom_; }

    /**
     * Get the PROMETHEE criteria choosing the positions.
     */
    [[nodiscard]] const RJSON& placementCriteria() const { return *placementCriteria_; }

    /**
     * Get the PROMETHEE criteria choosing the objects kept by the crossover.
     */
    [[nodiscard]] const RJSON& selectionCriteria() const { return *selectionCriteria_; }

private:

    //-----------------------------------------------------
    const RGA2DProblem* problem_;           ///< Problem.
    std::string heuristic_ = "bottom_left"; ///< Heuristic.
    bool allOrientations_ = false;          ///< Try all the orientations?
    bool weightedDistances_ = false;        ///< Weighted lengths?
    bool firstRandom_ = true;               ///< First object chosen randomly?
    std::shared_ptr<const RJSON> placementCriteria_;    ///< Criteria of the positions.
    std::shared_ptr<const RJSON> selectionCriteria_;    ///< Criteria of the selection.
};


}  //------- End of namespace evocomb ----------------------------------------------
