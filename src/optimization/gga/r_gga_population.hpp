/*

    EvoComb

    RGGAPopulation.hpp

    Population of the Grouping Genetic Algorithm - Header.

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
#include <memory>
#include <utility>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "app/r_logger.hpp"
#include "ga/r_ga_population.hpp"
#include "gga/r_gga_chromo.hpp"
#include "tools/r_json.hpp"


//------------------------------------------------------------------------------
namespace evocomb {
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
 * The RGGAPopulation class provides the population of a grouping genetic algorithm. It owns the context (the objects to
 * group and the parameters of the GGA, read from the "gga" object of the configuration), gives it to the chromosomes,
 * and creates the thread-dependent data with the constructor ThreadData(RLogger&, const Context&) followed by a call to
 * ThreadData::init.
 *
 * It is used with RGAInst as any population:
 * @code
 * auto config = RJSON::parse(R"({"ga": {"nb_threads": 4, "mutation_frequency": 5},
 *                                "gga": {"heuristic": "first_fit"}})");
 * RGGAPopulation<MyChromo> population(32, MyContext(objs), config);
 * RGAInst inst(logger, config);
 * inst.init(&population);
 * inst.run(100);
 * const auto& best = population.best();
 * @endcode
 * The population must not be moved once initialized (the chromosomes point to its context).
 * @tparam C                     Class of the chromosome (it must inherit from RGGAChromo).
 * @short GGA population.
 */
template<class C>
    class RGGAPopulation : public RGAPopulation<C>
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
     * @param context            Context (objects to group and parameters).
     * @param config             Configuration (the "gga" object is read).
     * @exception RGAException   The objects or the parameters are invalid.
     */
    RGGAPopulation(const std::size_t nb, Context context, const RJSON& config) :
        RGAPopulation<C>(nb),
        context_(std::move(context))
    {
        context_.verifyObjs();
        context_.configure(config);
    }

    /**
     * Get the context.
     */
    [[nodiscard]] const Context& context() const { return context_; }

    //-----------------------------------------------------
    RGGAPopulation(const RGGAPopulation&) = delete;
    RGGAPopulation& operator=(const RGGAPopulation&) = delete;

protected:

    /**
     * Initialize a chromosome and give it the context.
     * @param logger             Logger.
     * @param chromo             Chromosome.
     * @param id                 Identifier of the chromosome.
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
