/*

    EvoComb

    RGGAHeuristic.hpp

    Grouping heuristics - Header.

    Copyright 1998-2026 by Pascal Francq (pascal@francq.info).
    Copyright 1998-2008 by the Université Libre de Bruxelles (ULB).

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
#include <string>
#include <vector>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "app/r_logger.hpp"
#include "ga/r_ga.hpp"
#include "gga/r_gga_context.hpp"
#include "gga/r_gga_groups.hpp"
#include "tools/r_concat.hpp"
#include "tools/r_std.hpp"


//------------------------------------------------------------------------------
namespace evocomb {
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
 * The RGGAHeuristic class provides the base class for the heuristics assigning the objects not yet assigned of a
 * solution (a set of groups, see RGGAGroups) to groups. A heuristic has access to the context of the GGA (for example
 * to use some data of the problem).
 *
 * The heuristic first builds the order in which the objects are treated (buildOrder, by default a random order), and
 * then assigns each object to the group returned by findGroup.
 *
 * A heuristic has an internal state (the order): an instance must not be used by two threads at the same time (each
 * thread has its own instances, see RGGAThreadData).
 * @tparam G                     Class of the groups.
 * @tparam O                     Class of the objects.
 * @tparam Ctx                   Class of the context.
 * @short Grouping heuristic.
 */
template<class G, class O, class Ctx = RGGAContext<O>>
    class RGGAHeuristic
{
public:

    /**
     * Class of the solutions.
     */
    using Groups = RGGAGroups<G, O>;

    /**
     * Construct the heuristic.
     * @param name               Name of the heuristic.
     * @param context            Context of the GGA (it must live as long as the heuristic).
     */
    RGGAHeuristic(std::string name, const Ctx& context) : name_(std::move(name)), context_(context) {}

    /**
     * Destruct the heuristic.
     */
    virtual ~RGGAHeuristic() = default;

    /**
     * Get the name of the heuristic.
     */
    [[nodiscard]] const std::string& name() const { return name_; }

    /**
     * Get the context.
     */
    [[nodiscard]] const Ctx& context() const { return context_; }

    /**
     * Assign all the objects not yet assigned of a solution.
     * @param logger             Logger.
     * @param groups             Solution.
     * @param random             Random generator.
     * @exception RGAException   No group was found for an object.
     */
    void run(RLogger& logger, Groups& groups, RGARandom& random)
    {
        mLogModule("r.gga");
        buildOrder(logger, groups, random);
        for (const auto obj : order_)
        {
            const auto id = findGroup(logger, groups, *obj);
            if (id == cNoRef)
                mThrowRGAException(name_, ": no group found for object ", obj->id());
            groups.insertObj(id, *obj);
        }
        postRun(logger, groups, random);
    }

    //-----------------------------------------------------
    RGGAHeuristic(const RGGAHeuristic&) = delete;
    RGGAHeuristic& operator=(const RGGAHeuristic&) = delete;

protected:

    /**
     * Build the order in which the objects not assigned are treated (in the vector order_). By default, the objects are
     * randomly ordered.
     * @param logger             Logger.
     * @param groups             Solution.
     * @param random             Random generator.
     */
    virtual void buildOrder([[maybe_unused]] RLogger& logger, Groups& groups, RGARandom& random)
    {
        groups.notAssigned(order_);
        std::shuffle(order_.begin(), order_.end(), random);
    }

    /**
     * Find a group for an object. The method can reserve a new group.
     * @param logger             Logger.
     * @param groups             Solution.
     * @param obj                Object.
     * @return the identifier of the group (cNoRef if no group can be found).
     */
    [[nodiscard]] virtual std::size_t findGroup(RLogger& logger, Groups& groups, const O& obj) = 0;

    /**
     * Method called once all the objects are assigned.
     * @param logger             Logger.
     * @param groups             Solution.
     * @param random             Random generator.
     */
    virtual void postRun([[maybe_unused]] RLogger& logger, [[maybe_unused]] Groups& groups, [[maybe_unused]] RGARandom& random) {}

    //-----------------------------------------------------
    std::vector<const O*> order_;           ///< Order of the objects to assign.

private:

    //-----------------------------------------------------
    std::string name_;                      ///< Name of the heuristic.
    const Ctx& context_;                    ///< Context.
};


//------------------------------------------------------------------------------
/**
 * The RGGAFirstFitHeuristic class provides the first-fit heuristic: each object is assigned to the first group used
 * that accepts it (RGGAGroup::canInsert), or to a new group.
 * @tparam G                     Class of the groups.
 * @tparam O                     Class of the objects.
 * @tparam Ctx                   Class of the context.
 * @short First-fit heuristic.
 */
template<class G, class O, class Ctx = RGGAContext<O>>
    class RGGAFirstFitHeuristic : public RGGAHeuristic<G, O, Ctx>
{
public:

    /**
     * Class of the solutions.
     */
    using Groups = RGGAGroups<G, O>;

    /**
     * Construct the heuristic.
     * @param context            Context of the GGA.
     * @param name               Name of the heuristic.
     */
    explicit RGGAFirstFitHeuristic(const Ctx& context, std::string name = "first_fit") : RGGAHeuristic<G, O, Ctx>(std::move(name), context) {}

protected:

    /**
     * Find the first group used accepting the object (a new group is reserved if none accepts it).
     * @param logger             Logger.
     * @param groups             Solution.
     * @param obj                Object.
     */
    [[nodiscard]] std::size_t findGroup([[maybe_unused]] RLogger& logger, Groups& groups, const O& obj) override
    {
        for (const auto id : groups.used())
            if (groups.group(id).canInsert(obj))
                return id;
        return groups.reserveGroup();
    }
};


}  //------- End of namespace evocomb ----------------------------------------------
