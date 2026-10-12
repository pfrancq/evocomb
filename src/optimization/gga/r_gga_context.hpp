/*

    EvoComb

    RGGAContext.hpp

    Context of the Grouping Genetic Algorithm - Header.

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
 * The RGGAContext class provides the data shared by all the chromosomes of a grouping genetic algorithm: the objects to
 * group and the parameters of the GGA. It is owned by the population (see RGGAPopulation) and must not be modified
 * during a run.
 *
 * The parameters are read from the "gga" object of the configuration (all of them are optional):
 * @code
 * {
 *   "gga": {
 *     "heuristic": "first_fit",          // Heuristic of the random construction and of the repair.
 *     "empty_modified_groups": true,     // Crossover: skip the groups having common objects.
 *     "local_optimize": true             // Local optimization before the repair.
 *   }
 * }
 * @endcode
 * A problem can derive from this class to add its own data (for example the capacity of the groups).
 * @tparam O                     Class of the objects.
 * @short GGA context.
 */
template<class O>
    class RGGAContext
{
public:

    /**
     * Construct the context.
     * @param objs               Objects to group (objs[i]->id() must be i). They are not owned by the context.
     */
    explicit RGGAContext(std::vector<const O*> objs = {}) : objs_(std::move(objs)) {}

    /**
     * Destruct the context.
     */
    virtual ~RGGAContext() = default;

    /**
     * Read the parameters from the "gga" object of a configuration (the parameters missing keep their values).
     * @param config             Configuration.
     * @exception RGAException   A parameter is invalid.
     */
    virtual void configure(const RJSON& config)
    {
        mLogModule("r.gga");
        const auto gga = config.find("gga");
        if (gga == nullptr)
            return;
        if (!gga->isObject())
            mThrowRGAException("'gga' must be a JSON object");
        if (const auto param = gga->find("heuristic"); param != nullptr)
        {
            if (!param->isString())
                mThrowRGAException("'heuristic' must be a string");
            heuristic_ = param->toString();
        }
        readBool(*gga, "empty_modified_groups", emptyModifiedGroups_);
        readBool(*gga, "local_optimize", localOptimize_);
    }

    /**
     * Verify the objects (their identifiers must be their indexes).
     * @exception RGAException   The objects are invalid.
     */
    void verifyObjs() const
    {
        mLogModule("r.gga");
        for (std::size_t i = 0; i < objs_.size(); ++i)
        {
            if (objs_[i] == nullptr)
                mThrowRGAException("Object ", i, " is null");
            if (objs_[i]->id() != i)
                mThrowRGAException("Object at index ", i, " has the identifier ", objs_[i]->id());
        }
    }

    /**
     * Get the objects to group.
     */
    [[nodiscard]] const std::vector<const O*>& objs() const { return objs_; }

    /**
     * Get the name of the heuristic used for the random construction and the repair of the chromosomes.
     */
    [[nodiscard]] const std::string& heuristic() const { return heuristic_; }

    /**
     * Look if, during a crossover, the groups of a parent having common objects with the groups inserted from the other
     * parent are skipped (true), or if only the common objects are skipped (false).
     */
    [[nodiscard]] bool emptyModifiedGroups() const { return emptyModifiedGroups_; }

    /**
     * Look if a local optimization is done before the repair of a chromosome (see RGGAChromo::localOptimize).
     */
    [[nodiscard]] bool localOptimize() const { return localOptimize_; }

protected:

    /**
     * Set the name of the heuristic (for example to give another default value in a derived class).
     * @param name               Name of the heuristic.
     */
    void heuristic(const std::string& name) { heuristic_ = name; }

    /**
     * Read an optional boolean parameter.
     * @param obj                JSON object.
     * @param key                Key of the parameter.
     * @param value              Value to set (unchanged if the parameter is missing).
     * @exception RGAException   The parameter is not a boolean.
     */
    static void readBool(const RJSON& obj, const std::string& key, bool& value)
    {
        mLogModule("r.gga");
        const auto param = obj.find(key);
        if (param == nullptr)
            return;
        if (!param->isBoolean())
            mThrowRGAException("'", key, "' must be a boolean");
        value = param->toBool();
    }

    /**
     * Read an optional real parameter.
     * @param obj                JSON object.
     * @param key                Key of the parameter.
     * @param value              Value to set (unchanged if the parameter is missing).
     * @exception RGAException   The parameter is not a number.
     */
    static void readDouble(const RJSON& obj, const std::string& key, double& value)
    {
        mLogModule("r.gga");
        const auto param = obj.find(key);
        if (param == nullptr)
            return;
        if (param->isDecimal())
            value = param->toDecimal();
        else if (param->isInteger())
            value = static_cast<double>(param->toInteger());
        else
            mThrowRGAException("'", key, "' must be a number");
    }

    /**
     * Read an optional positive integer parameter.
     * @param obj                JSON object.
     * @param key                Key of the parameter.
     * @param value              Value to set (unchanged if the parameter is missing).
     * @exception RGAException   The parameter is not a positive integer.
     */
    static void readSize(const RJSON& obj, const std::string& key, std::size_t& value)
    {
        mLogModule("r.gga");
        const auto param = obj.find(key);
        if (param == nullptr)
            return;
        if ((!param->isInteger()) || (param->toInteger() < 0))
            mThrowRGAException("'", key, "' must be a positive integer");
        value = static_cast<std::size_t>(param->toInteger());
    }

private:

    //-----------------------------------------------------
    std::vector<const O*> objs_;            ///< Objects to group.
    std::string heuristic_ = "first_fit";   ///< Name of the heuristic.
    bool emptyModifiedGroups_ = true;       ///< Skip the modified groups in the crossover?
    bool localOptimize_ = true;             ///< Local optimization before the repair?
};


}  //------- End of namespace evocomb ----------------------------------------------
