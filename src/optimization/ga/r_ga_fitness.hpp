/*

    EvoComb

    RGAFitness.hpp

    Fitness strategies of the Genetic Algorithm - Header.

    Copyright 2026 by Pascal Francq (pascal@francq.info).

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/



//------------------------------------------------------------------------------
#pragma once


//------------------------------------------------------------------------------
// include files for ANSI C/C++
#include <cstddef>
#include <memory>
#include <string>
#include <vector>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "app/r_logger.hpp"
#include "tools/r_json.hpp"
#include "multicriteria/r_prom_kernel.hpp"


//------------------------------------------------------------------------------
namespace evocomb {
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
 * The RGAFitness class provides a strategy computing the fitness of the chromosomes from the values of their criteria
 * (see RGAChromo::criterion). The chromosomes only compute the values of their criteria, so that the same class of
 * chromosome can be used with different strategies.
 *
 * A strategy is created from the "fitness" object of the configuration of the GA (see create).
 * @short Fitness strategy.
 */
class RGAFitness
{
public:

    /**
     * Construct the strategy.
     */
    RGAFitness() = default;

    /**
     * Destruct the strategy.
     */
    virtual ~RGAFitness() = default;

    /**
     * Get the name of the strategy (as used in the configuration).
     */
    [[nodiscard]] virtual std::string name() const = 0;

    /**
     * Look if the fitness computed must be maximized.
     * @return `true` if the fitness must be maximized, `false` if the strategy supports both.
     */
    [[nodiscard]] virtual bool requiresMaximize() const { return false; }

    /**
     * Compute the fitness of a set of solutions. The fitness values computed by a single call must be comparable with
     * each other (but not necessarily with those of another call).
     * @param logger             Logger.
     * @param criteria           Values of the criteria of each solution.
     * @param fitness            Fitness of each solution (same size as criteria).
     * @exception RGAException   The criteria are invalid.
     */
    virtual void compute(RLogger& logger, const std::vector<const std::vector<double>*>& criteria, std::vector<double>& fitness) = 0;

    /**
     * Create a strategy from a JSON configuration:
     * @code
     * {"type": "direct"}                                // Default.
     * {"type": "promethee", "criteria": [ ... ]}        // See RPromKernel::addCriteria.
     * @endcode
     * @param logger             Logger.
     * @param config             Configuration (if null, the direct strategy is created).
     * @exception RGAException   The configuration is invalid.
     */
    [[nodiscard]] static std::unique_ptr<RGAFitness> create(RLogger& logger, const RJSON* config);
};


//------------------------------------------------------------------------------
/**
 * The RGADirectFitness class provides the direct strategy: the fitness of a chromosome is the value of its first
 * criterion (criterion 0).
 * @short Direct fitness.
 */
class RGADirectFitness : public RGAFitness
{
public:

    /**
     * Get the name of the strategy ("direct").
     */
    [[nodiscard]] std::string name() const override { return "direct"; }

    /**
     * Compute the fitness: the value of the first criterion.
     * @param logger             Logger.
     * @param criteria           Values of the criteria of each solution.
     * @param fitness            Fitness of each solution.
     * @exception RGAException   A solution has no criterion.
     */
    void compute(RLogger& logger, const std::vector<const std::vector<double>*>& criteria, std::vector<double>& fitness) override;
};


//------------------------------------------------------------------------------
/**
 * The RGAPromFitness class provides a multi-criteria strategy: the fitness of a chromosome is its net flow (Fi)
 * computed by PROMETHEE II over all the solutions of a call (the population and the best chromosome ever computed). The
 * fitness must therefore be maximized.
 * @short PROMETHEE fitness.
 */
class RGAPromFitness : public RGAFitness
{
public:

    /**
     * Construct the strategy.
     * @param logger             Logger.
     * @param criteria           Criteria (see RPromKernel::addCriteria).
     * @exception RException     The criteria are invalid.
     */
    RGAPromFitness(RLogger& logger, const RJSON& criteria);

    /**
     * Get the name of the strategy ("promethee").
     */
    [[nodiscard]] std::string name() const override { return "promethee"; }

    /**
     * The Fi must be maximized.
     */
    [[nodiscard]] bool requiresMaximize() const override { return true; }

    /**
     * Get the PROMETHEE kernel (for example to look at the criteria).
     */
    [[nodiscard]] const RPromKernel& kernel() const { return kernel_; }

    /**
     * Compute the fitness: the Fi of each solution.
     * @param logger             Logger.
     * @param criteria           Values of the criteria of each solution (as many values as PROMETHEE criteria).
     * @param fitness            Fitness of each solution.
     * @exception RGAException   The number of values of a solution is invalid.
     */
    void compute(RLogger& logger, const std::vector<const std::vector<double>*>& criteria, std::vector<double>& fitness) override;

private:

    //-----------------------------------------------------
    RPromKernel kernel_;                    ///< PROMETHEE kernel.
};


}  //------- End of namespace evocomb ----------------------------------------------
