/*

    EvoComb

    RHGAContext.hpp

    Context of the Hierarchical Genetic Algorithm - Header.

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
#include <string>
#include <utility>
#include <vector>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "app/r_logger.hpp"
#include "ga/r_ga.hpp"
#include "tools/r_json.hpp"


//------------------------------------------------------------------------------
namespace evocomb {
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
 * The RHGAContext class provides the data shared by all the chromosomes of a Hierarchical Genetic Algorithm: the
 * objects to place in the tree and the parameters of the HGA. It is owned by the population (see RHGAPopulation) and
 * must not be modified during a run.
 *
 * The parameters are read from the "hga" object of the configuration (all of them are optional):
 * @code
 * {
 *   "hga": {
 *     "heuristic": "first_node"          // Heuristic of the random construction and of the repair.
 *   }
 * }
 * @endcode
 * A problem can derive from this class to add its own data.
 * @tparam O                     Class of the objects.
 * @short HGA context.
 */
template<class O>
    class RHGAContext
{
public:

    /**
     * Construct the context.
     * @param objs               Objects to place (objs[i]->id() must be i). They are not owned by the context.
     */
    explicit RHGAContext(std::vector<const O*> objs = {}) : objs_(std::move(objs)) {}

    /**
     * Destruct the context.
     */
    virtual ~RHGAContext() = default;

    /**
     * Read the parameters from the "hga" object of a configuration (the parameters missing keep their values).
     * @param config             Configuration.
     * @exception RGAException   A parameter is invalid.
     */
    virtual void configure(const RJSON& config)
    {
        mLogModule("r.hga");
        const auto hga = config.find("hga");
        if (hga == nullptr)
            return;
        if (!hga->isObject())
            mThrowRGAException("'hga' must be a JSON object");
        if (const auto param = hga->find("heuristic"); param != nullptr)
        {
            if (!param->isString())
                mThrowRGAException("'heuristic' must be a string");
            heuristic_ = param->toString();
        }
    }

    /**
     * Verify the objects: their identifiers must be their indexes, and each one must have at least one attribute.
     * @exception RGAException   The objects are invalid.
     */
    void verifyObjs() const
    {
        mLogModule("r.hga");
        for (std::size_t i = 0; i < objs_.size(); ++i)
        {
            if (objs_[i] == nullptr)
                mThrowRGAException("Object ", i, " is null");
            if (objs_[i]->id() != i)
                mThrowRGAException("Object at index ", i, " has the identifier ", objs_[i]->id());
            if (objs_[i]->attrs().empty())
                mThrowRGAException("Object ", i, " has no attribute");
        }
    }

    /**
     * Get the objects to place.
     */
    [[nodiscard]] const std::vector<const O*>& objs() const { return objs_; }

    /**
     * Get the name of the heuristic used for the random construction and the repair of the chromosomes.
     */
    [[nodiscard]] const std::string& heuristic() const { return heuristic_; }

protected:

    /**
     * Set the name of the heuristic (for example to give another default value in a derived class).
     * @param name               Name of the heuristic.
     */
    void heuristic(const std::string& name) { heuristic_ = name; }

private:

    //-----------------------------------------------------
    std::vector<const O*> objs_;            ///< Objects to place.
    std::string heuristic_ = "first_node";  ///< Name of the heuristic.
};


}  //------- End of namespace evocomb ----------------------------------------------
