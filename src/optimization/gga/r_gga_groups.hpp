/*

    EvoComb

    RGGAGroups.hpp

    Groups of the Grouping Genetic Algorithm - Header.

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
#include <deque>
#include <map>
#include <type_traits>
#include <utility>
#include <vector>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "app/r_logger.hpp"
#include "ga/r_ga.hpp"
#include "gga/r_gga_group.hpp"
#include "tools/r_concat.hpp"
#include "tools/r_std.hpp"


//------------------------------------------------------------------------------
namespace evocomb {
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
 * The RGGAGroups class provides a set of groups containing objects: it is the representation of a solution of a
 * grouping problem (see RGGAChromo).
 *
 * The objects are identified by their index in a vector of objects shared by all the solutions (object i has the
 * identifier i). The groups are identified by their index in the set: a group is either reserved (used by the solution)
 * or free. The order of the reserved groups (used) is significant for the genetic operators.
 *
 * The objects are always inserted or deleted through the set (insertObj, deleteObj, etc.), which maintains the
 * assignment of the objects and calls the hooks of the groups.
 *
 * The set is a value: it can be copied (the objects are shared, not copied).
 * @tparam G                     Class of the groups (it must inherit from RGGAGroup<O>).
 * @tparam O                     Class of the objects.
 * @short Groups.
 */
template<class G, class O>
    class RGGAGroups
{
    static_assert(std::is_base_of_v<RGGAGroup<O>, G>, "G must inherit from RGGAGroup<O>");

public:

    /**
     * Class of the groups.
     */
    using Group = G;

    /**
     * Class of the objects.
     */
    using Object = O;

    /**
     * Construct the groups.
     */
    RGGAGroups() = default;

    RGGAGroups(const RGGAGroups&) = default;
    RGGAGroups(RGGAGroups&&) noexcept = default;
    RGGAGroups& operator=(const RGGAGroups&) = default;
    RGGAGroups& operator=(RGGAGroups&&) noexcept = default;

    /**
     * Destruct the groups.
     */
    virtual ~RGGAGroups() = default;

    /**
     * Initialize the groups with the objects to group (all the groups are released).
     * @param objs               Objects (objs[i]->id() must be i). The vector must live as long as the groups.
     */
    void initGroups(const std::vector<const O*>& objs)
    {
        objs_ = &objs;
        groups_.clear();
        clearGroups();
    }

    /**
     * Release all the groups (no object is assigned anymore).
     */
    void clearGroups()
    {
        for (const auto id : used_)
        {
            auto& grp = groups_[id];
            grp.objs_.clear();
            grp.reserved_ = false;
            grp.clear();
        }
        used_.clear();
        objsAss_.assign(nbObjs(), cNoRef);
        nbAssigned_ = 0;
        ord_.clear();
    }

    /**
     * Get the number of objects to group.
     */
    [[nodiscard]] std::size_t nbObjs() const { return objs_ ? objs_->size() : 0; }

    /**
     * Get the objects to group.
     */
    [[nodiscard]] const std::vector<const O*>& objs() const { return *objs_; }

    /**
     * Get an object.
     * @param id                 Identifier of the object.
     */
    [[nodiscard]] const O& obj(const std::size_t id) const { return *(*objs_)[id]; }

    /**
     * Get the number of objects assigned to a group.
     */
    [[nodiscard]] std::size_t nbAssigned() const { return nbAssigned_; }

    /**
     * Get the number of objects not assigned to a group.
     */
    [[nodiscard]] std::size_t nbNotAssigned() const { return nbObjs() - nbAssigned_; }

    /**
     * Get the objects not assigned (in the order of their identifiers).
     * @param objs               Vector filled with the objects (its previous content is erased).
     */
    void notAssigned(std::vector<const O*>& objs) const
    {
        objs.clear();
        for (std::size_t id = 0; id < objsAss_.size(); ++id)
            if (objsAss_[id] == cNoRef)
                objs.push_back((*objs_)[id]);
    }

    /**
     * Reserve a group.
     * @param id                 Identifier of the group. If cNoRef, the first free group is chosen (a new one is
     *                           created if necessary).
     * @return the identifier of the group reserved.
     * @exception RGAException   The group is already reserved.
     */
    std::size_t reserveGroup(std::size_t id = cNoRef)
    {
        mLogModule("r.gga");
        if (id == cNoRef)
        {
            const auto it = std::find_if(groups_.begin(), groups_.end(), [](const G& grp) { return !grp.reserved_; });
            id = static_cast<std::size_t>(it - groups_.begin());
        }
        while (groups_.size() <= id)
            groups_.push_back(newGroup(groups_.size()));
        auto& grp = groups_[id];
        if (grp.reserved_)
            mThrowRGAException("Group ", id, " is already reserved");
        grp.reserved_ = true;
        used_.push_back(id);
        return id;
    }

    /**
     * Release a group: its objects are deleted and it is not used anymore.
     * @param id                 Identifier of the group.
     */
    void releaseGroup(const std::size_t id)
    {
        auto& grp = groups_[id];
        if (!grp.reserved_)
            return;
        deleteObjs(id);
        used_.erase(std::find(used_.begin(), used_.end(), id));
        grp.reserved_ = false;
        grp.clear();
    }

    /**
     * Insert an object into a group (the method postInsert of the group is called).
     * @param id                 Identifier of the group.
     * @param obj                Object.
     * @exception RGAException   The group is not reserved or the object is already assigned.
     */
    void insertObj(const std::size_t id, const O& obj)
    {
        mLogModule("r.gga");
        auto& grp = groups_[id];
        if (!grp.reserved_)
            mThrowRGAException("Group ", id, " is not reserved");
        if (objsAss_[obj.id()] != cNoRef)
            mThrowRGAException("Object ", obj.id(), " is already assigned to group ", objsAss_[obj.id()]);
        grp.objs_.push_back(&obj);
        objsAss_[obj.id()] = id;
        ++nbAssigned_;
        grp.postInsert(obj);
    }

    /**
     * Insert all the objects of a group (for example of another solution) into an empty group (the methods preInsertAll
     * and, if necessary, postInsert of the group are called).
     * @param from               Group used as source.
     * @param id                 Identifier of the destination group.
     * @exception RGAException   The group is not reserved or not empty, or an object is already assigned.
     */
    void insertObjs(const G& from, const std::size_t id)
    {
        mLogModule("r.gga");
        auto& grp = groups_[id];
        if (!grp.reserved_)
            mThrowRGAException("Group ", id, " is not reserved");
        if (!grp.objs_.empty())
            mThrowRGAException("Group ", id, " is not empty");
        const auto callPostInsert = grp.preInsertAll(from);
        for (const auto obj : from.objs_)
        {
            if (objsAss_[obj->id()] != cNoRef)
                mThrowRGAException("Object ", obj->id(), " is already assigned to group ", objsAss_[obj->id()]);
            grp.objs_.push_back(obj);
            objsAss_[obj->id()] = id;
            ++nbAssigned_;
            if (callPostInsert)
                grp.postInsert(*obj);
        }
    }

    /**
     * Delete an object from its group (the method postDelete of the group is called).
     * @param obj                Object.
     * @exception RGAException   The object is not assigned.
     */
    void deleteObj(const O& obj)
    {
        mLogModule("r.gga");
        const auto id = objsAss_[obj.id()];
        if (id == cNoRef)
            mThrowRGAException("Object ", obj.id(), " is not assigned");
        auto& grp = groups_[id];
        grp.objs_.erase(std::find(grp.objs_.begin(), grp.objs_.end(), &obj));
        objsAss_[obj.id()] = cNoRef;
        --nbAssigned_;
        grp.postDelete(obj);
    }

    /**
     * Delete all the objects of a group (the methods preDeleteAll and, if necessary, postDelete of the group are
     * called).
     * @param id                 Identifier of the group.
     */
    void deleteObjs(const std::size_t id)
    {
        auto& grp = groups_[id];
        if (grp.objs_.empty())
            return;
        const auto callPostDelete = grp.preDeleteAll();
        const auto objs = std::move(grp.objs_);
        grp.objs_.clear();
        for (const auto obj : objs)
        {
            objsAss_[obj->id()] = cNoRef;
            --nbAssigned_;
            if (callPostDelete)
                grp.postDelete(*obj);
        }
    }

    /**
     * Get a group.
     * @param id                 Identifier of the group.
     */
    [[nodiscard]] G& group(const std::size_t id) { return groups_[id]; }

    /**
     * Get a group.
     * @param id                 Identifier of the group.
     */
    [[nodiscard]] const G& group(const std::size_t id) const { return groups_[id]; }

    /**
     * Get the identifiers of the groups used (in their order).
     */
    [[nodiscard]] const std::vector<std::size_t>& used() const { return used_; }

    /**
     * Get the number of groups used.
     */
    [[nodiscard]] std::size_t nbUsed() const { return used_.size(); }

    /**
     * Get a group used.
     * @param pos                Position in the groups used.
     */
    [[nodiscard]] const G& usedGroup(const std::size_t pos) const { return groups_[used_[pos]]; }

    /**
     * Exchange the positions of two groups used.
     * @param pos1               First position.
     * @param pos2               Second position.
     */
    void exchangeUsed(const std::size_t pos1, const std::size_t pos2) { std::swap(used_[pos1], used_[pos2]); }

    /**
     * Get the identifier of the group of an object.
     * @param id                 Identifier of the object.
     * @return cNoRef if the object is not assigned.
     */
    [[nodiscard]] std::size_t groupId(const std::size_t id) const { return objsAss_[id]; }

    /**
     * Get the group of an object.
     * @param obj                Object.
     * @return nullptr if the object is not assigned.
     */
    [[nodiscard]] const G* groupOf(const O& obj) const
    {
        const auto id = objsAss_[obj.id()];
        return (id == cNoRef) ? nullptr : &groups_[id];
    }

    /**
     * Compute a normalized assignment of the objects, where the groups are numbered in the order of their first object.
     * It is used by sameGrouping and must be called after each modification of the groups (the genetic operators of
     * RGGAChromo do it).
     */
    void computeOrd()
    {
        std::vector<std::size_t> newIds(groups_.size(), cNoRef);
        std::size_t nbGroups = 0;
        ord_.assign(objsAss_.size(), cNoRef);
        for (std::size_t i = 0; i < objsAss_.size(); ++i)
        {
            const auto id = objsAss_[i];
            if (id == cNoRef)
                continue;
            if (newIds[id] == cNoRef)
                newIds[id] = nbGroups++;
            ord_[i] = newIds[id];
        }
    }

    /**
     * Look if two solutions represent the same grouping (whatever the identifiers and the order of the groups).
     * computeOrd must have been called on both.
     * @param groups             Solution to compare with.
     */
    [[nodiscard]] bool sameGrouping(const RGGAGroups& groups) const { return ord_ == groups.ord_; }

    /**
     * Compute the adjusted Rand index between two solutions. It measures the similarity of the groupings: 1 if they are
     * identical, around 0 for independent groupings (it can be negative). Only the objects assigned in both solutions
     * are considered.
     * @param groups             Solution to compare with.
     * @return the index (-1 if it cannot be computed).
     */
    [[nodiscard]] double adjustedRandIndex(const RGGAGroups& groups) const
    {
        // Contingency table between the groups of both solutions
        std::map<std::pair<std::size_t, std::size_t>, double> table;
        std::map<std::size_t, double> rows;
        std::map<std::size_t, double> cols;
        double nbObjs = 0.0;
        for (std::size_t i = 0; i < objsAss_.size(); ++i)
        {
            const auto g1 = objsAss_[i];
            const auto g2 = (i < groups.objsAss_.size()) ? groups.objsAss_[i] : cNoRef;
            if ((g1 == cNoRef) || (g2 == cNoRef))
                continue;
            table[{g1, g2}] += 1.0;
            rows[g1] += 1.0;
            cols[g2] += 1.0;
            nbObjs += 1.0;
        }
        if (nbObjs < 2.0)
            return -1.0;

        const auto pairs = [](const double n) { return n * (n - 1.0) / 2.0; };
        double a = 0.0;
        double b = 0.0;
        double c = 0.0;
        for (const auto& [key, n] : table)
            a += pairs(n);
        for (const auto& [key, n] : cols)
            b += pairs(n);
        for (const auto& [key, n] : rows)
            c += pairs(n);
        const auto d = pairs(nbObjs);
        const auto num = a - (b * c) / d;
        const auto den = 0.5 * (b + c) - (b * c) / d;
        if (den < cEpsi)
            return -1.0;
        return num / den;
    }

    /**
     * Verify the coherence of the groups and of the assignment of the objects (and call the method verify of each group
     * used).
     * @exception RGAException   The groups are incoherent.
     */
    void verifyGroups() const
    {
        mLogModule("r.gga");
        std::size_t nb = 0;
        for (const auto id : used_)
        {
            const auto& grp = groups_[id];
            if (!grp.reserved_)
                mThrowRGAException("Group ", id, " is used but not reserved");
            if (grp.id_ != id)
                mThrowRGAException("Group ", id, " has the identifier ", grp.id_);
            for (const auto obj : grp.objs_)
                if (objsAss_[obj->id()] != id)
                    mThrowRGAException("Object ", obj->id(), " is in group ", id, " but assigned to ", objsAss_[obj->id()]);
            nb += grp.objs_.size();
            grp.verify();
        }
        if (nb != nbAssigned_)
            mThrowRGAException(nb, " objects in the groups but ", nbAssigned_, " assigned");
        for (std::size_t i = 0; i < objsAss_.size(); ++i)
            if ((objsAss_[i] != cNoRef) && (!groups_[objsAss_[i]].reserved_))
                mThrowRGAException("Object ", i, " is assigned to the free group ", objsAss_[i]);
    }

protected:

    /**
     * Create a new (free) group. By default, the group is constructed with its identifier. A derived class overrides
     * this method if the groups need other parameters.
     * @param id                 Identifier of the group.
     * @exception RGAException   G cannot be constructed from an identifier (the method must then be overridden).
     */
    [[nodiscard]] virtual G newGroup(const std::size_t id) const
    {
        if constexpr (std::is_constructible_v<G, std::size_t>)
            return G(id);
        else
        {
            mLogModule("r.gga");
            mThrowRGAException("G is not constructible from an identifier: newGroup must be overridden");
        }
    }

private:

    //-----------------------------------------------------
    const std::vector<const O*>* objs_ = nullptr;   ///< Objects to group (shared).
    std::deque<G> groups_;                  ///< All the groups (the identifier is the index).
    std::vector<std::size_t> used_;         ///< Identifiers of the groups used.
    std::vector<std::size_t> objsAss_;      ///< Group of each object (cNoRef if not assigned).
    std::size_t nbAssigned_ = 0;            ///< Number of objects assigned.
    std::vector<std::size_t> ord_;          ///< Normalized assignment (see computeOrd).
};


}  //------- End of namespace evocomb ----------------------------------------------
