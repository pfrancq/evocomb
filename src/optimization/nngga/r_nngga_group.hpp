/*

    EvoComb

    RNNGGAGroup.hpp

    Group of the Nearest Neighbors Grouping Genetic Algorithm - Header.

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


//------------------------------------------------------------------------------
// include files for EvoComb
#include "gga/r_gga_group.hpp"
#include "nngga/r_nngga_context.hpp"


//------------------------------------------------------------------------------
namespace evocomb {
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
 * The RNNGGAGroup class provides a group of the Nearest Neighbors Grouping Genetic Algorithm. An object can be inserted
 * in the group if:
 * - The group does not contain the maximal number of objects.
 * - It has no disagreement ratio greater or equal to the minimal
 *   disagreement with an object of the group.
 * - It has no similarity lower or equal to the minimal similarity with an
 *   object of the group (among its nearest neighbors).
 * Since the neighbor lists are limited in size, they may be asymmetric: the last two conditions are therefore checked
 * in both directions (the lists of the object and those of the objects of the group).
 * @tparam O                     Class of the objects.
 * @short NNGGA Group.
 */
template<class O>
    class RNNGGAGroup : public RGGAGroup<O>
{
public:

    /**
     * Construct the group.
     * @param id                 Identifier of the group.
     * @param context            Context (it must live as long as the group).
     */
    RNNGGAGroup(const std::size_t id, const RNNGGAContext<O>& context) :
        RGGAGroup<O>(id),
        context_(&context)
    {
    }

    /**
     * Get the context.
     */
    [[nodiscard]] const RNNGGAContext<O>& context() const { return *context_; }

    /**
     * Look if an object can be inserted in the group (see the class description).
     * @param obj                Object.
     */
    [[nodiscard]] bool canInsert(const O& obj) const override
    {
        // An empty group accepts any object
        if (this->nbObjs() == 0)
            return true;

        // Maximal number of objects
        if (context_->nbMaxObjs() && this->nbObjs() >= context_->nbMaxObjs())
            return false;

        // No object of the group with a high disagreement ratio
        const auto& data = context_->data();
        for (const auto& [id, value] : data.disagreements(obj.id()))
        {
            if (value < context_->minDisagreement())
                break;
            if (this->isIn(id))
                return false;
        }

        // No object of the group with a low similarity (the lowest similarities are at the end)
        const auto& sims = data.sims(obj.id()).values();
        for (auto it = sims.rbegin(); it != sims.rend() && it->value_ <= context_->minSimLevel(); ++it)
            if (this->isIn(it->id_))
                return false;

        // Same tests from the point of view of the objects of the group: the neighbor lists are limited in size and may
        // therefore be asymmetric
        for (const auto member : this->objs())
        {
            if (const auto* value = data.disagreements(member->id()).find(obj.id()); value && value->value_ >= context_->minDisagreement())
                return false;
            if (const auto* value = data.sims(member->id()).find(obj.id()); value && value->value_ <= context_->minSimLevel())
                return false;
        }

        return true;
    }

private:

    //-----------------------------------------------------
    const RNNGGAContext<O>* context_;       ///< Context.
};


}  //------- End of namespace evocomb ----------------------------------------------
