/*

    EvoComb

    RSGGAHeuristic.hpp

    Heuristic of the Similarity-based Grouping Genetic Algorithm - Header.

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
#include <limits>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "app/r_logger.hpp"
#include "gga/r_gga_heuristic.hpp"
#include "sgga/r_sgga_context.hpp"
#include "sgga/r_sgga_operators.hpp"


//------------------------------------------------------------------------------
namespace evocomb {
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
 * The RSGGAHeuristic class provides the heuristic of the Similarity-based Grouping Genetic Algorithm ("sg_first_fit").
 * For each object (in a random order):
 * 1. If an object with which it has an agreement ratio greater or equal to
 *    the minimal agreement is already assigned (see RSGGAContext::closeObjs),
 *    the object is put in the same group (if this group accepts it,
 *    RGGAGroup::canInsert).
 * 2. Otherwise, the object is put in the group accepting it that contains
 *    the most similar object.
 * 3. Otherwise, a new group is created.
 *
 * Once all the objects are assigned, the groups having less than the minimal number of objects, or containing only one
 * social object, are emptied (see RSGGAOperators::emptyGroups).
 * @tparam G                     Class of the groups.
 * @tparam O                     Class of the objects.
 * @short SGGA Heuristic.
 */
template<class G, class O>
    class RSGGAHeuristic : public RGGAHeuristic<G, O, RSGGAContext<O>>
{
public:

    /**
     * Class of the solutions.
     */
    using Groups = RGGAGroups<G, O>;

    /**
     * Construct the heuristic.
     * @param context            Context of the SGGA.
     */
    explicit RSGGAHeuristic(const RSGGAContext<O>& context) :
        RGGAHeuristic<G, O, RSGGAContext<O>>("sg_first_fit", context)
    {
    }

protected:

    /**
     * Find a group for an object (see the class description).
     * @param logger             Logger.
     * @param groups             Solution.
     * @param obj                Object.
     */
    [[nodiscard]] std::size_t findGroup([[maybe_unused]] RLogger& logger, Groups& groups, const O& obj) override
    {
        // A close object is already assigned -> same group
        if (const auto grp = RSGGAOperators<G, O>::closeGroup(this->context(), groups, obj); grp != cNoRef)
            return grp;

        // Group accepting the object and containing the most similar object
        std::size_t grp = cNoRef;
        double best = -std::numeric_limits<double>::infinity();
        for (const auto id : groups.used())
        {
            double maxSim;
            if (!groups.group(id).canInsert(obj, maxSim))
                continue;
            if ((grp == cNoRef) || (maxSim > best))
            {
                best = maxSim;
                grp = id;
            }
        }
        if (grp != cNoRef)
            return grp;

        // No group was found -> create a new one
        return groups.reserveGroup();
    }

    /**
     * Empty the groups with too few objects or with a single social object (see the class description).
     * @param logger             Logger.
     * @param groups             Solution.
     * @param random             Random generator.
     */
    void postRun([[maybe_unused]] RLogger& logger, Groups& groups, [[maybe_unused]] RGARandom& random) override
    {
        [[maybe_unused]] mLogModule("r.sgga");
        [[maybe_unused]] const auto nb = RSGGAOperators<G, O>::emptyGroups(this->context(), groups, work_, true);
        mLogDebug(logger, nb, " object(s) moved from small groups");
    }

private:

    //-----------------------------------------------------
    RSGGAWork<O> work_;                     ///< Temporary structures.
};


}  //------- End of namespace evocomb ----------------------------------------------
