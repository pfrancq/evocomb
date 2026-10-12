/*

    EvoComb

    RNNGGAContext.hpp

    Context of the Nearest Neighbors Grouping Genetic Algorithm - Header.

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
#include "gga/r_gga_context.hpp"
#include "nngga/r_nngga_data.hpp"
#include "tools/r_json.hpp"


//------------------------------------------------------------------------------
namespace evocomb {
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
 * The RNNGGAContext class provides the context of the Nearest Neighbors Grouping Genetic Algorithm: the objects, their
 * data (see RNNGGAData) and the parameters. The parameters are read from the "nngga" object of the configuration (all
 * of them are optional):
 * @code
 * {
 *   "nngga": {
 *     "min_sim_level": 0.0,        // Objects with a lower similarity cannot be grouped together.
 *     "min_agreement": 1.0,        // Objects with a higher agreement ratio should be grouped together.
 *     "min_disagreement": 1.0,     // Objects with a higher disagreement ratio cannot be grouped together.
 *     "nb_min_objs": 0,            // Minimal number of objects per group.
 *     "nb_max_objs": 0             // Maximal number of objects per group (0 = no limit).
 *   }
 * }
 * @endcode
 * The default heuristic is "nn_first_fit" (see RNNGGAHeuristic).
 * @tparam O                     Class of the objects.
 * @short NNGGA Context.
 */
template<class O>
    class RNNGGAContext : public RGGAContext<O>
{
public:

    /**
     * Construct the context.
     * @param objs               Objects to group (objs[i]->id() must be i).
     * @param data               Data of the objects (it must live as long as the context).
     */
    RNNGGAContext(std::vector<const O*> objs, const RNNGGAData& data) :
        RGGAContext<O>(std::move(objs)),
        data_(&data)
    {
        this->heuristic("nn_first_fit");
    }

    /**
     * Read the parameters from the "gga" and "nngga" objects of a configuration.
     * @param config             Configuration.
     * @exception RGAException   A parameter is invalid, or the data do not correspond to the objects.
     */
    void configure(const RJSON& config) override
    {
        mLogModule("r.nngga");
        RGGAContext<O>::configure(config);
        if (data_->nbObjs() != this->objs().size())
            mThrowRGAException("The data concern ", data_->nbObjs(), " objects instead of ", this->objs().size());
        const auto nngga = config.find("nngga");
        if (nngga == nullptr)
            return;
        if (!nngga->isObject())
            mThrowRGAException("'nngga' must be a JSON object");
        this->readDouble(*nngga, "min_sim_level", minSimLevel_);
        this->readDouble(*nngga, "min_agreement", minAgreement_);
        this->readDouble(*nngga, "min_disagreement", minDisagreement_);
        this->readSize(*nngga, "nb_min_objs", nbMinObjs_);
        this->readSize(*nngga, "nb_max_objs", nbMaxObjs_);
    }

    /**
     * Get the data of the objects.
     */
    [[nodiscard]] const RNNGGAData& data() const { return *data_; }

    /**
     * Get the minimal similarity: two objects whose similarity is lower or equal cannot be grouped together.
     */
    [[nodiscard]] double minSimLevel() const { return minSimLevel_; }

    /**
     * Get the minimal agreement ratio: an object is preferably grouped with the objects with which it has a greater or
     * equal agreement ratio.
     */
    [[nodiscard]] double minAgreement() const { return minAgreement_; }

    /**
     * Get the minimal disagreement ratio: two objects whose disagreement ratio is greater or equal cannot be grouped
     * together.
     */
    [[nodiscard]] double minDisagreement() const { return minDisagreement_; }

    /**
     * Get the minimal number of objects per group.
     */
    [[nodiscard]] std::size_t nbMinObjs() const { return nbMinObjs_; }

    /**
     * Get the maximal number of objects per group (0 means no limit).
     */
    [[nodiscard]] std::size_t nbMaxObjs() const { return nbMaxObjs_; }

private:

    //-----------------------------------------------------
    const RNNGGAData* data_;                ///< Data of the objects.
    double minSimLevel_ = 0.0;              ///< Minimal similarity.
    double minAgreement_ = 1.0;             ///< Minimal agreement ratio.
    double minDisagreement_ = 1.0;          ///< Minimal disagreement ratio.
    std::size_t nbMinObjs_ = 0;             ///< Minimal number of objects per group.
    std::size_t nbMaxObjs_ = 0;             ///< Maximal number of objects per group (0 = no limit).
};


}  //------- End of namespace evocomb ----------------------------------------------
