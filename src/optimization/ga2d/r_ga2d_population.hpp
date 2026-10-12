/*

    EvoComb

    RGA2DPopulation.hpp

    Population of the 2D Placement Genetic Algorithm - Header.

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
#include <memory>
#include <utility>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "app/r_logger.hpp"
#include "ga/r_ga_population.hpp"
#include "ga2d/r_ga2d_chromo.hpp"
#include "tools/r_json.hpp"


//------------------------------------------------------------------------------
namespace evocomb {
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
 * The RGA2DPopulation class provides the population of the 2D placement genetic algorithm. It owns the context (the
 * parameters are read from the "ga2d" object of the configuration), gives it to the chromosomes, and creates the
 * thread-dependent data with the constructor ThreadData(RLogger&, const RGA2DContext&) followed by a call to
 * ThreadData::init:
 * @code
 * auto config = RJSON::parse(R"({"ga": {"maximize": false}, "ga2d": {"heuristic": "edge"}})");
 * evocomb::RGA2DPopulation<> population(16, evocomb::RGA2DContext(problem), config);
 * evocomb::RGAInst inst(logger, config);
 * inst.init(&population);
 * inst.run(50);
 * @endcode
 * The population must not be moved once initialized.
 * @tparam C                     Class of the chromosome (it must inherit from RGA2DChromo).
 * @short 2D Placement Population.
 */
template<class C = RGA2DChromo>
    class RGA2DPopulation : public RGAPopulation<C>
{
public:

    /**
     * Class of the context.
     */
    using Context = typename C::Context;

    /**
     * Class of the thread-dependent data.
     */
    using ThreadData = typename C::ThreadData;

    /**
     * Construct the population.
     * @param nb                 Number of chromosomes.
     * @param context            Context.
     * @param config             Configuration (the "ga2d" object is read).
     * @exception RGAException   A parameter is invalid.
     */
    RGA2DPopulation(const std::size_t nb, Context context, const RJSON& config) :
        RGAPopulation<C>(nb),
        context_(std::move(context))
    {
        context_.configure(config);
    }

    /**
     * Get the context.
     */
    [[nodiscard]] const Context& context() const { return context_; }

    //-----------------------------------------------------
    RGA2DPopulation(const RGA2DPopulation&) = delete;
    RGA2DPopulation& operator=(const RGA2DPopulation&) = delete;

protected:

    /**
     * Initialize a chromosome and give it the context.
     * @param logger             Logger.
     * @param chromo             Chromosome.
     * @param id                 Identifier.
     */
    void initChromo(RLogger& logger, C& chromo, const std::size_t id) override
    {
        chromo.init(logger, id);
        chromo.setContext(context_);
    }

    /**
     * Create the thread-dependent data of a thread.
     * @param logger             Logger.
     * @param thread             Index of the thread.
     */
    std::unique_ptr<ThreadData> newThreadData(RLogger& logger, [[maybe_unused]] const std::size_t thread) override
    {
        auto data = std::make_unique<ThreadData>(logger, context_);
        data->init(logger);
        return data;
    }

private:

    //-----------------------------------------------------
    Context context_;                       ///< Context.
};


}  //------- End of namespace evocomb ----------------------------------------------
