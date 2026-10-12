/*

    EvoComb

    RNNGGAHeuristic.hpp

    Heuristic of the Nearest Neighbors Grouping Genetic Algorithm - Header.

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
#include <algorithm>
#include <cstddef>
#include <utility>
#include <vector>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "app/r_logger.hpp"
#include "gga/r_gga_heuristic.hpp"
#include "nngga/r_nngga_context.hpp"


//------------------------------------------------------------------------------
namespace evocomb {
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
 * The RNNGGAHeuristic class provides the heuristic of the Nearest Neighbors Grouping Genetic Algorithm
 * ("nn_first_fit"). For each object (in a random order):
 * 1. If an object with which it has an agreement ratio greater or equal to
 *    the minimal agreement is already assigned, the object is put in the same
 *    group (if this group accepts it, RGGAGroup::canInsert).
 * 2. Otherwise, the groups containing its nearest neighbors are ordered by
 *    decreasing number of neighbors, and the object is put in the first one
 *    accepting it (RGGAGroup::canInsert).
 * 3. Otherwise, a new group is created.
 *
 * Once all the objects are assigned, the groups having less than the minimal number of objects, or containing only one
 * social object, are emptied: each of their objects is moved to the group of its first agreeing (or, if none, nearest)
 * neighbor in another group accepting it, if any.
 * @tparam G                     Class of the groups.
 * @tparam O                     Class of the objects.
 * @short NNGGA Heuristic.
 */
template<class G, class O>
    class RNNGGAHeuristic : public RGGAHeuristic<G, O, RNNGGAContext<O>>
{
public:

    /**
     * Class of the solutions.
     */
    using Groups = RGGAGroups<G, O>;

    /**
     * Construct the heuristic.
     * @param context            Context of the NNGGA.
     */
    explicit RNNGGAHeuristic(const RNNGGAContext<O>& context) :
        RGGAHeuristic<G, O, RNNGGAContext<O>>("nn_first_fit", context)
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
        const auto& context = this->context();
        const auto& data = context.data();

        // An agreeing object is already assigned -> same group (if it accepts the object)
        for (const auto& [id, value] : data.agreements(obj.id()))
        {
            if (value < context.minAgreement())
                break;
            if (const auto grp = groups.groupId(id); (grp != cNoRef) && groups.group(grp).canInsert(obj))
                return grp;
        }

        // Count the nearest neighbors in each group (in the order of the first neighbor found)
        counts_.clear();
        for (const auto& [id, value] : data.sims(obj.id()))
        {
            const auto grp = groups.groupId(id);
            if (grp == cNoRef)
                continue;
            const auto it = std::find_if(counts_.begin(), counts_.end(), [grp](const auto& item) { return item.first == grp; });
            if (it == counts_.end())
                counts_.emplace_back(grp, 1);
            else
                ++it->second;
        }

        // First group accepting the object, by decreasing number of neighbors
        std::stable_sort(counts_.begin(), counts_.end(), [](const auto& a, const auto& b) { return a.second > b.second; });
        for (const auto& [grp, nb] : counts_)
            if (groups.group(grp).canInsert(obj))
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
        [[maybe_unused]] mLogModule("r.nngga");
        const auto& context = this->context();
        const auto& data = context.data();

        // First group of a neighbor of an object that is not a given group and that accepts the object
        const auto otherGroup = [&groups](const RMaxVector& neighbors, const std::size_t current, const O& obj) -> std::size_t {
            for (const auto& [id, value] : neighbors)
                if (const auto grp = groups.groupId(id); (grp != cNoRef) && (grp != current) && groups.group(grp).canInsert(obj))
                    return grp;
            return cNoRef;
        };

        toDel_.clear();
        const auto used = groups.used();
        for (const auto id : used)
        {
            const auto& grp = groups.group(id);
            if (grp.nbObjs() >= context.nbMinObjs())
            {
                // Only the groups containing a single social object are treated
                if (grp.nbObjs() != 1)
                    continue;
                if (!data.isSocial(grp.objs().front()->id()))
                    continue;
            }

            // Move the objects in other groups (if possible)
            toReassign_ = grp.objs();
            for (const auto obj : toReassign_)
            {
                auto target = otherGroup(data.agreements(obj->id()), id, *obj);
                if (target == cNoRef)
                    target = otherGroup(data.sims(obj->id()), id, *obj);
                if (target == cNoRef)
                    continue;
                mLogDebug(logger, "Object ", obj->id(), " moved from group ", id, " to group ", target);
                groups.deleteObj(*obj);
                groups.insertObj(target, *obj);
            }
            if (groups.group(id).nbObjs() == 0)
                toDel_.push_back(id);
        }

        // Release the empty groups
        for (const auto id : toDel_)
            groups.releaseGroup(id);
    }

private:

    //-----------------------------------------------------
    std::vector<std::pair<std::size_t, std::size_t>> counts_;   ///< Number of neighbors per group.
    std::vector<const O*> toReassign_;      ///< Objects to reassign.
    std::vector<std::size_t> toDel_;        ///< Groups to release.
};


}  //------- End of namespace evocomb ----------------------------------------------
