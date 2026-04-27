/*

    EvoComb

    RSGGAOperators.hpp

    Operators of the Similarity-based Grouping Genetic Algorithm - Header.

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
#include <limits>
#include <type_traits>
#include <utility>
#include <vector>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "ga/r_ga.hpp"
#include "gga/r_gga_groups.hpp"
#include "sgga/r_sgga_context.hpp"
#include "sgga/r_sgga_group.hpp"
#include "tools/r_std.hpp"


//------------------------------------------------------------------------------
namespace evocomb {
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
 * The RSGGAWork class provides the temporary structures used by the operators of the SGGA (see RSGGAOperators). An
 * instance must not be used by two threads at the same time.
 * @tparam O                     Class of the objects.
 * @short SGGA temporary structures.
 */
template<class O>
    struct RSGGAWork
{
    std::vector<const O*> order;            ///< Order of the objects (k-Means).
    std::vector<const O*> protos;           ///< Prototypes (k-Means).
    std::vector<const O*> objs1;            ///< Temporary objects.
    std::vector<const O*> objs2;            ///< Temporary objects.
    std::vector<std::pair<double, const O*>> sims;  ///< Objects with their similarity with a centroid.
    std::vector<std::size_t> groups;        ///< Temporary identifiers of groups.
};


//------------------------------------------------------------------------------
/**
 * The RSGGAOperators class provides the operators of the Similarity-based Grouping Genetic Algorithm. They work on any
 * solution (RGGAGroups), which allows applying them to the chromosomes and to temporary solutions (see
 * RSGGAChromo::optimize). The groups must inherit from RSGGAGroup<O>.
 *
 * It is a static class.
 * @tparam G                     Class of the groups.
 * @tparam O                     Class of the objects.
 * @short SGGA Operators.
 */
template<class G, class O>
    class RSGGAOperators
{
    static_assert(std::is_base_of_v<RSGGAGroup<O>, G>, "G must inherit from RSGGAGroup<O>");

public:

    /**
     * Class of the solutions.
     */
    using Groups = RGGAGroups<G, O>;

    /**
     * Class of the context.
     */
    using Context = RSGGAContext<O>;

    /**
     * Class of the temporary structures.
     */
    using Work = RSGGAWork<O>;

    /**
     * Compute the three criteria of a solution:
     * - Criterion 0, similarity "J": the average similarity between the
     *   objects and the centroid of their group, divided by the highest
     *   average similarity between the centroid of a group and the centroids
     *   of the other groups (if there are at least two groups).
     * - Criterion 1: the average of the agreement measures of the groups
     *   (see RSGGAGroup::avgAgreement).
     * - Criterion 2: the average of the disagreement measures of the groups
     *   (see RSGGAGroup::avgDisagreement).
     * @param groups             Solution.
     * @param criteria           Vector receiving the three values.
     */
    static void evaluate(const Groups& groups, std::vector<double>& criteria)
    {
        criteria.assign(3, 0.0);
        const auto& used = groups.used();
        if (used.empty())
            return;
        const auto nb = static_cast<double>(used.size());
        double intra = 0.0;
        double agree = 0.0;
        double disagree = 0.0;
        double maxInter = -std::numeric_limits<double>::infinity();
        for (const auto id1 : used)
        {
            const auto& grp1 = groups.group(id1);
            intra += grp1.avgIntraSim();
            agree += grp1.avgAgreement();
            disagree += grp1.avgDisagreement();
            if (used.size() < 2)
                continue;
            double inter = 0.0;
            for (const auto id2 : used)
                if (id1 != id2)
                    inter += grp1.relSim(*groups.group(id2).centroid());
            maxInter = std::max(maxInter, inter / (nb - 1.0));
        }
        criteria[0] = intra / nb;
        if (used.size() > 1)
            criteria[0] /= std::max(maxInter, cEpsi);
        criteria[1] = agree / nb;
        criteria[2] = disagree / nb;
    }

    /**
     * Find the group of the first object "close" to a given one (see RSGGAContext::closeObjs) that accepts it.
     * @param context            Context.
     * @param groups             Solution.
     * @param obj                Object.
     * @return cNoRef if no such group exists.
     */
    [[nodiscard]] static std::size_t closeGroup(const Context& context, const Groups& groups, const O& obj)
    {
        for (const auto& [id, value] : context.closeObjs(obj.id()))
            if (const auto grp = groups.groupId(id); (grp != cNoRef) && groups.group(grp).canInsert(obj))
                return grp;
        return cNoRef;
    }

    /**
     * Perform a k-Means on a solution: the centroids of the groups are the initial prototypes, and the objects are
     * reallocated (see reAllocate) until the ratio of modified prototypes is lower or equal to the convergence
     * threshold or the maximal number of iterations is reached.
     * @param context            Context.
     * @param groups             Solution (it must contain at least one group).
     * @param work               Temporary structures.
     * @param random             Random generator.
     * @return the number of iterations.
     */
    static std::size_t kMeans(const Context& context, Groups& groups, Work& work, RGARandom& random)
    {
        if (groups.nbUsed() == 0)
            return 0;
        work.order = groups.objs();
        std::shuffle(work.order.begin(), work.order.end(), random);
        std::size_t iter = 0;
        while (iter < context.maxKMeans())
        {
            ++iter;
            reAllocate(context, groups, work);

            // Count the groups whose centroid is not the prototype anymore (the new groups count as modified)
            const auto& used = groups.used();
            std::size_t nbChanged = 0;
            for (std::size_t i = 0; i < used.size(); ++i)
                if ((i >= work.protos.size()) || (groups.group(used[i]).centroid() != work.protos[i]))
                    ++nbChanged;
            if (static_cast<double>(nbChanged) / static_cast<double>(used.size()) <= context.convergence())
                break;
        }
        return iter;
    }

    /**
     * Reallocate the objects: the centroids of the groups become the prototypes of new groups, and the objects (in the
     * order of work.order) are assigned to the group of their first close object (see closeGroup) or, otherwise, to the
     * group accepting them whose prototype is the most similar. An object that cannot be inserted in any group becomes
     * the prototype of a new group.
     * @param context            Context.
     * @param groups             Solution.
     * @param work               Temporary structures (work.order contains all the objects; work.protos receives the
     *                           prototypes).
     */
    static void reAllocate(const Context& context, Groups& groups, Work& work)
    {
        // Prototypes
        work.protos.clear();
        for (const auto id : groups.used())
            work.protos.push_back(groups.group(id).centroid());

        // One group per prototype (whose centroid is fixed)
        groups.clearGroups();
        for (const auto proto : work.protos)
        {
            const auto grp = groups.reserveGroup();
            groups.insertObj(grp, *proto);
            groups.group(grp).lockCentroid(*proto);
        }

        // Assign the other objects
        for (const auto obj : work.order)
        {
            if (groups.groupId(obj->id()) != cNoRef)
                continue;
            auto grp = closeGroup(context, groups, *obj);
            if (grp == cNoRef)
            {
                double best = -std::numeric_limits<double>::infinity();
                for (const auto id : groups.used())
                {
                    const auto& group = groups.group(id);
                    if (!group.canInsert(*obj))
                        continue;
                    if (const auto sim = group.relSim(*obj); (grp == cNoRef) || (sim > best))
                    {
                        best = sim;
                        grp = id;
                    }
                }
            }
            if (grp != cNoRef)
                groups.insertObj(grp, *obj);
            else
            {
                grp = groups.reserveGroup();
                groups.insertObj(grp, *obj);
                groups.group(grp).lockCentroid(*obj);
            }
        }

        // The centroids are computed again
        for (const auto id : groups.used())
            groups.group(id).unlockCentroid();
    }

    /**
     * Divide the group containing the two most dissimilar objects (among the groups with at least three objects): each
     * object goes with the most similar of these two objects.
     * @param context            Context.
     * @param groups             Solution.
     * @param work               Temporary structures.
     * @return `true` if a group was divided.
     */
    static bool divideWorstObjects(const Context& context, Groups& groups, Work& work)
    {
        const auto& data = context.data();
        std::size_t grp = cNoRef;
        const O* worst1 = nullptr;
        const O* worst2 = nullptr;
        double min = std::numeric_limits<double>::infinity();
        for (const auto id : groups.used())
        {
            const auto& objs = groups.group(id).objs();
            if (objs.size() < 3)
                continue;
            for (std::size_t i = 0; i < objs.size(); ++i)
                for (std::size_t j = i + 1; j < objs.size(); ++j)
                    if (const auto sim = data.sim(objs[i]->id(), objs[j]->id()); sim < min)
                    {
                        min = sim;
                        grp = id;
                        worst1 = objs[i];
                        worst2 = objs[j];
                    }
        }
        if (grp == cNoRef)
            return false;

        // Split the objects
        work.objs1.assign(1, worst1);
        work.objs2.assign(1, worst2);
        for (const auto obj : groups.group(grp).objs())
        {
            if ((obj == worst1) || (obj == worst2))
                continue;
            if (data.sim(worst1->id(), obj->id()) > data.sim(worst2->id(), obj->id()))
                work.objs1.push_back(obj);
            else
                work.objs2.push_back(obj);
        }
        groups.releaseGroup(grp);
        for (const auto* objs : {&work.objs1, &work.objs2})
        {
            const auto id = groups.reserveGroup();
            for (const auto obj : *objs)
                groups.insertObj(id, *obj);
        }
        return true;
    }

    /**
     * Merge the two groups having the most similar centroids: the objects of the first one are moved into the second
     * one (if it accepts them).
     * @param context            Context.
     * @param groups             Solution.
     * @param work               Temporary structures.
     * @return `true` if at least one object was moved.
     */
    static bool mergeBestGroups([[maybe_unused]] const Context& context, Groups& groups, Work& work)
    {
        const auto& used = groups.used();
        if (used.size() < 2)
            return false;
        std::size_t grp1 = cNoRef;
        std::size_t grp2 = cNoRef;
        double max = -std::numeric_limits<double>::infinity();
        for (const auto id1 : used)
            for (const auto id2 : used)
            {
                if (id1 == id2)
                    continue;
                if (const auto sim = groups.group(id1).relSim(*groups.group(id2).centroid()); (grp1 == cNoRef) || (sim > max))
                {
                    max = sim;
                    grp1 = id1;
                    grp2 = id2;
                }
            }

        // Move the objects accepted by grp2
        work.objs1 = groups.group(grp1).objs();
        auto moved = false;
        for (const auto obj : work.objs1)
        {
            if (!groups.group(grp2).canInsert(*obj))
                continue;
            groups.deleteObj(*obj);
            groups.insertObj(grp2, *obj);
            moved = true;
        }
        if (groups.group(grp1).nbObjs() == 0)
            groups.releaseGroup(grp1);
        return moved;
    }

    /**
     * Empty some groups by moving each of their objects to the group accepting it whose centroid is the most similar
     * (if any). The groups emptied are those containing a single social object and, if withMinObjs is true, those
     * containing less than the minimal number of objects. The groups becoming empty are released.
     * @param context            Context.
     * @param groups             Solution.
     * @param work               Temporary structures.
     * @param withMinObjs        Empty also the groups with too few objects?
     * @return the number of objects moved.
     */
    static std::size_t emptyGroups(const Context& context, Groups& groups, Work& work, const bool withMinObjs)
    {
        const auto& data = context.data();
        std::size_t nbMoved = 0;
        work.groups = groups.used();
        for (const auto id : work.groups)
        {
            const auto& grp = groups.group(id);
            const auto nb = grp.nbObjs();
            const auto single = (nb == 1) && data.isSocial(grp.objs().front()->id());
            const auto tooSmall = withMinObjs && (nb < context.nbMinObjs());
            if ((!single) && (!tooSmall))
                continue;
            work.objs1 = grp.objs();
            for (const auto obj : work.objs1)
            {
                std::size_t target = cNoRef;
                double best = -std::numeric_limits<double>::infinity();
                for (const auto other : groups.used())
                {
                    if (other == id)
                        continue;
                    const auto& group = groups.group(other);
                    if ((group.nbObjs() == 0) || (!group.canInsert(*obj)))
                        continue;
                    if (const auto sim = group.relSim(*obj); (target == cNoRef) || (sim > best))
                    {
                        best = sim;
                        target = other;
                    }
                }
                if (target == cNoRef)
                    continue;
                groups.deleteObj(*obj);
                groups.insertObj(target, *obj);
                ++nbMoved;
            }
            if (groups.group(id).nbObjs() == 0)
                groups.releaseGroup(id);
        }
        return nbMoved;
    }

    /**
     * Local optimization: try to insert some objects not assigned into the existing groups, possibly by removing up to
     * three objects of a group, if this increases its average similarity with its centroid (see optimizeGroup). At most
     * four passes are done.
     * @param context            Context.
     * @param groups             Solution.
     * @param objs               Objects not assigned (updated: the objects inserted are removed and the objects removed
     *                           from the groups are added).
     * @param work               Temporary structures.
     * @param random             Random generator.
     * @return `true` if the solution was modified.
     */
    static bool localOptimize(const Context& context, Groups& groups, std::vector<const O*>& objs, Work& work, RGARandom& random)
    {
        std::shuffle(objs.begin(), objs.end(), random);
        auto modified = false;
        for (std::size_t pass = 0; (pass < 4) && (!objs.empty()); ++pass)
        {
            auto optimized = false;
            for (const auto id : groups.used())
                if (optimizeGroup(context, groups, id, objs, work))
                {
                    optimized = true;
                    break;
                }
            if (!optimized)
                break;
            modified = true;
        }
        return modified;
    }

    /**
     * Try to optimize a group (with at least two objects) with some objects not assigned. Two objects (then a single
     * one) accepted by the group are searched such that, once inserted, the average similarity of the objects with the
     * centroid increases, possibly after having removed up to three of the objects the least similar to the centroid.
     * The first such modification found is done.
     * @param context            Context.
     * @param groups             Solution.
     * @param id                 Identifier of the group.
     * @param objs               Objects not assigned (updated).
     * @param work               Temporary structures.
     * @return `true` if the group was modified.
     */
    static bool optimizeGroup(const Context& context, Groups& groups, const std::size_t id, std::vector<const O*>& objs, Work& work)
    {
        const auto& grp = groups.group(id);
        const auto nb = grp.nbObjs();
        if (nb < 2)
            return false;
        const auto centroid = grp.centroid();
        const auto avg = grp.avgIntraSim();

        // Objects of the group (except the centroid) by increasing similarity with the centroid
        work.sims.clear();
        for (const auto obj : grp.objs())
            if (obj != centroid)
                work.sims.emplace_back(grp.relSim(*obj), obj);
        std::sort(work.sims.begin(), work.sims.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
        const auto base = avg * static_cast<double>(nb - 1);

        // Look for the number of objects to remove (0 to 3) such that adding objects with a total similarity 'sim'
        // increases the average
        const auto nbToRemove = [&](const std::size_t nbAdd, const double sim) -> std::size_t {
            double removed = 0.0;
            for (std::size_t k = 0; (k <= 3) && (k < nb); ++k)
            {
                if (k > 0)
                    removed += work.sims[k - 1].first;
                if (context.nbMaxObjs() && (nb - k + nbAdd > context.nbMaxObjs()))
                    continue;
                const auto newAvg = (base + sim - removed) / static_cast<double>(nb - 1 - k + nbAdd);
                if (newAvg > avg)
                    return k;
            }
            return cNoRef;
        };

        // Try to add two objects, then one
        const O* add1 = nullptr;
        const O* add2 = nullptr;
        std::size_t k = cNoRef;
        for (std::size_t i = 0; (i < objs.size()) && (k == cNoRef); ++i)
        {
            if (!grp.canInsert(*objs[i]))
                continue;
            for (std::size_t j = i + 1; (j < objs.size()) && (k == cNoRef); ++j)
                if (grp.canInsert(*objs[j]) && context.compatible(*objs[i], *objs[j]))
                    if (k = nbToRemove(2, grp.relSim(*objs[i]) + grp.relSim(*objs[j])); k != cNoRef)
                    {
                        add1 = objs[i];
                        add2 = objs[j];
                    }
        }
        for (std::size_t i = 0; (i < objs.size()) && (k == cNoRef); ++i)
            if (grp.canInsert(*objs[i]))
                if (k = nbToRemove(1, grp.relSim(*objs[i])); k != cNoRef)
                    add1 = objs[i];
        if (k == cNoRef)
            return false;

        // Remove the k objects the least similar to the centroid, and insert the new ones (if they are still accepted)
        for (std::size_t i = 0; i < k; ++i)
        {
            groups.deleteObj(*work.sims[i].second);
            objs.push_back(work.sims[i].second);
        }
        for (const auto obj : {add1, add2})
        {
            if ((obj == nullptr) || (!groups.group(id).canInsert(*obj)))
                continue;
            groups.insertObj(id, *obj);
            objs.erase(std::find(objs.begin(), objs.end(), obj));
        }
        return true;
    }

    //-----------------------------------------------------
    RSGGAOperators() = delete;
};


}  //------- End of namespace evocomb ----------------------------------------------
