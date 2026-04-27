/*

    EvoComb

    RGGAGroup.hpp

    Group of the Grouping Genetic Algorithm - Header.

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
#include <vector>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "tools/r_std.hpp"


//------------------------------------------------------------------------------
namespace evocomb {
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// forward declaration
template<class G, class O> class RGGAGroups;


//------------------------------------------------------------------------------
/**
 * The RGGAGroup class provides the base class for the groups of the grouping genetic algorithm (see RGGAGroups). A
 * group contains objects of type O, which must provide a method returning their identifier:
 * @code
 * std::size_t id() const;       // In [0, number of objects[.
 * @endcode
 *
 * A group is a value: it is copied with the chromosome containing it. It does not know its owner: the objects are
 * inserted and deleted through the owner (RGGAGroups), which calls the hooks of the group (canInsert, postInsert,
 * postDelete, etc.). A derived class overrides these hooks to maintain its own information (for example the total size
 * of the objects).
 * @tparam O                     Class of the objects.
 * @short Group.
 */
template<class O>
    class RGGAGroup
{
public:

    /**
     * Construct the group.
     * @param id                 Identifier of the group.
     */
    explicit RGGAGroup(const std::size_t id = cNoRef) : id_(id) {}

    RGGAGroup(const RGGAGroup&) = default;
    RGGAGroup(RGGAGroup&&) noexcept = default;
    RGGAGroup& operator=(const RGGAGroup&) = default;
    RGGAGroup& operator=(RGGAGroup&&) noexcept = default;

    /**
     * Destruct the group.
     */
    virtual ~RGGAGroup() = default;

    /**
     * Get the identifier of the group.
     */
    [[nodiscard]] std::size_t id() const { return id_; }

    /**
     * Look if the group is reserved (used by the chromosome).
     */
    [[nodiscard]] bool isReserved() const { return reserved_; }

    /**
     * Get the number of objects in the group.
     */
    [[nodiscard]] std::size_t nbObjs() const { return objs_.size(); }

    /**
     * Get the objects of the group.
     */
    [[nodiscard]] const std::vector<const O*>& objs() const { return objs_; }

    /**
     * Look if an object is in the group.
     * @param id                 Identifier of the object.
     */
    [[nodiscard]] bool isIn(const std::size_t id) const
    {
        return std::any_of(objs_.begin(), objs_.end(), [id](const O* obj) { return obj->id() == id; });
    }

    /**
     * Look if an object can be inserted in the group. By default, it returns always true.
     * @param obj                Object.
     */
    [[nodiscard]] virtual bool canInsert([[maybe_unused]] const O& obj) const { return true; }

    /**
     * Look if a group is "compatible" with the current one (used by the crossover: a group of a parent is not inserted
     * if it is incompatible with a group kept from the other parent). By default, it returns always true.
     * @param grp                Group.
     */
    [[nodiscard]] virtual bool isCompatible([[maybe_unused]] const RGGAGroup& grp) const { return true; }

    /**
     * Method called after an object was inserted in the group.
     * @param obj                Object.
     */
    virtual void postInsert([[maybe_unused]] const O& obj) {}

    /**
     * Method called before all the objects of a group are inserted into the current one (which is empty). It can be
     * used to copy some information.
     * @param from               Group used as source.
     * @return `true` if postInsert must be called for each object inserted (default).
     */
    virtual bool preInsertAll([[maybe_unused]] const RGGAGroup& from) { return true; }

    /**
     * Method called after an object was deleted from the group.
     * @param obj                Object.
     */
    virtual void postDelete([[maybe_unused]] const O& obj) {}

    /**
     * Method called before all the objects of the group are deleted.
     * @return `true` if postDelete must be called for each object deleted (default).
     */
    virtual bool preDeleteAll() { return true; }

    /**
     * Method called when the group is released (it contains no more objects). It can be used to reset some information.
     */
    virtual void clear() {}

    /**
     * Verify the specific information of the group. An exception should be thrown if the group is invalid. The
     * coherence between the group and the assignment of the objects is verified by RGGAGroups.
     */
    virtual void verify() const {}

private:

    //-----------------------------------------------------
    std::size_t id_;                        ///< Identifier.
    bool reserved_ = false;                 ///< Is the group reserved?
    std::vector<const O*> objs_;            ///< Objects of the group.

    //-----------------------------------------------------
    template<class, class> friend class RGGAGroups;
};


}  //------- End of namespace evocomb ----------------------------------------------
