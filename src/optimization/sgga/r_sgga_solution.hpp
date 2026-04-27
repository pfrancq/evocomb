/*

    EvoComb

    RSGGASolution.hpp

    Solution of the Similarity-based Grouping Genetic Algorithm - Header.

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
#include <vector>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "gga/r_gga_groups.hpp"
#include "sgga/r_sgga_context.hpp"


//------------------------------------------------------------------------------
namespace evocomb {
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
 * The RSGGASolution class provides a temporary solution of the Similarity-based Grouping Genetic Algorithm: a set of
 * groups with the values of its criteria. The optimization of a chromosome (see RSGGAChromo::optimize) copies the
 * groups of the chromosome into such solutions to modify them.
 * @tparam G                     Class of the groups.
 * @tparam O                     Class of the objects.
 * @short SGGA Solution.
 */
template<class G, class O>
    class RSGGASolution : public RGGAGroups<G, O>
{
public:

    /**
     * Class of the solutions.
     */
    using Groups = RGGAGroups<G, O>;

    /**
     * Initialize the solution with a context.
     * @param context            Context (it must live as long as the solution).
     */
    void init(const RSGGAContext<O>& context)
    {
        context_ = &context;
        this->initGroups(context.objs());
    }

    /**
     * Copy the groups of another solution (sharing the same context).
     * @param groups             Solution used as source.
     */
    void assign(const Groups& groups) { Groups::operator=(groups); }

    /**
     * Get the values of the criteria (see RSGGAOperators::evaluate).
     */
    [[nodiscard]] std::vector<double>& criteria() { return criteria_; }

    /**
     * Get the values of the criteria (see RSGGAOperators::evaluate).
     */
    [[nodiscard]] const std::vector<double>& criteria() const { return criteria_; }

protected:

    /**
     * Create a group with the context.
     * @param id                 Identifier of the group.
     */
    [[nodiscard]] G newGroup(const std::size_t id) const override { return G(id, *context_); }

private:

    //-----------------------------------------------------
    const RSGGAContext<O>* context_ = nullptr;  ///< Context.
    std::vector<double> criteria_;          ///< Values of the criteria.
};


}  //------- End of namespace evocomb ----------------------------------------------
