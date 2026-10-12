/*

    EvoComb

    RGAChromo.hpp

    Chromosome of the Genetic Algorithm - Header.

    Copyright 2007-2026 by Pascal Francq (pascal@francq.info).
    Copyright 2007-2008 by the Université Libre de Bruxelles (ULB).

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
#include <vector>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "app/r_logger.hpp"
#include "r_ga.hpp"


//------------------------------------------------------------------------------
namespace evocomb {
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// forward declaration
template <class C> class RGAPopulation;


//------------------------------------------------------------------------------
/**
 * The RGAChromo class provides the base class for the chromosomes managed by the GA (see RGAPopulation). All the
 * operators do nothing by default, except strongMutation which calls mutation.
 *
 * A chromosome must be default-constructible and copy-assignable: the GA copies chromosomes (for example to store the
 * best one ever computed). The identifier of a chromosome is never modified by such a copy.
 *
 * To detect identical chromosomes (see the "handle_duplicates" parameter of RGAInst), a class of chromosome can provide
 * the method:
 * @code
 * bool isSame(const Chromo& chromo) const;
 * @endcode
 * Without it, two chromosomes are never considered as identical.
 *
 * The operators can be executed in parallel (on different chromosomes). Each operator receives:
 * - The "thread-dependent" data of the thread executing it (an instance of
 *   T): temporary working structures that are created once per thread (see
 *   RGAPopulation::newThreadData) instead of at each call. They are never
 *   used by two threads at the same time.
 * - The random generator that the operator must use (for the operators
 *   needing random numbers). It is seeded for each operation by the GA, so
 *   that the result of the GA does not depend on the number of threads.
 *
 * An operator must only modify its own chromosome (and the thread-dependent data it receives).
 *
 * The method evaluate computes the values of one or more criteria (with criterion). The fitness of the chromosome is
 * then computed from these values by the fitness strategy of the GA (see RGAFitness): for example, the value of the
 * first criterion (direct strategy) or the Fi of PROMETHEE II. The same class of chromosome can therefore be used with
 * any strategy. Whether the fitness must be maximized (default) or minimized is a parameter of the GA instance (see
 * RGAInst).
 * @tparam T                     Class of the thread-dependent data.
 * @short Generic chromosome.
 */
template <class T = RGANoThreadData>
    class RGAChromo
{
public:

    /**
     * Class of the thread-dependent data.
     */
    using ThreadData = T;

    /**
     * Construct the chromosome.
     */
    RGAChromo() = default;

    /**
     * Destruct the chromosome.
     */
    virtual ~RGAChromo() = default;

    /**
     * Initialize the chromosome (called once when the population is initialized).
     * @param logger             Logger.
     * @param id                 Identifier of the chromosome.
     */
    virtual void init([[maybe_unused]] RLogger& logger, const std::size_t id) { id_ = id; }

    /**
     * Get the identifier of the chromosome.
     */
    [[nodiscard]] std::size_t id() const { return id_; }

    /**
     * Build a random (valid) solution. It is called once, at the beginning of the first run.
     * @param logger             Logger.
     * @param data               Thread-dependent data.
     * @param random             Random generator.
     */
    virtual void randomConstruct([[maybe_unused]] RLogger& logger, [[maybe_unused]] T& data, [[maybe_unused]] RGARandom& random) {}

    /**
     * Do a mutation.
     * @param logger             Logger.
     * @param data               Thread-dependent data.
     * @param random             Random generator.
     */
    virtual void mutation([[maybe_unused]] RLogger& logger, [[maybe_unused]] T& data, [[maybe_unused]] RGARandom& random) {}

    /**
     * Do a strong mutation (by default, a normal mutation is done).
     * @param logger             Logger.
     * @param data               Thread-dependent data.
     * @param random             Random generator.
     */
    virtual void strongMutation(RLogger& logger, T& data, RGARandom& random) { mutation(logger, data, random); }

    /**
     * Do an inversion.
     * @param logger             Logger.
     * @param data               Thread-dependent data.
     * @param random             Random generator.
     */
    virtual void inversion([[maybe_unused]] RLogger& logger, [[maybe_unused]] T& data, [[maybe_unused]] RGARandom& random) {}

    /**
     * Modify the chromosome because it is identical to another (better) one (see the "handle_duplicates" parameter of
     * RGAInst). By default, a mutation is done.
     * @param logger             Logger.
     * @param data               Thread-dependent data.
     * @param random             Random generator.
     */
    virtual void modify(RLogger& logger, T& data, RGARandom& random) { mutation(logger, data, random); }

    /**
     * Perform a local optimization. If enabled in the instance, it is called each time the chromosome is modified by
     * the crossover, the mutation or the inversion operator.
     * @param logger             Logger.
     * @param data               Thread-dependent data.
     * @param random             Random generator.
     */
    virtual void optimize([[maybe_unused]] RLogger& logger, [[maybe_unused]] T& data, [[maybe_unused]] RGARandom& random) {}

    /**
     * Compute the values of the criteria of the chromosome (with criterion). The fitness is then computed by the
     * fitness strategy of the GA.
     * @param logger             Logger.
     * @param data               Thread-dependent data.
     */
    virtual void evaluate([[maybe_unused]] RLogger& logger, [[maybe_unused]] T& data) {}

    /**
     * Get the fitness of the chromosome (as computed by the fitness strategy after the last evaluation, 0 before).
     */
    [[nodiscard]] double fitness() const { return fitness_; }

    /**
     * Get the values of the criteria (as computed by the last call to evaluate).
     */
    [[nodiscard]] const std::vector<double>& criteria() const { return criteria_; }

    /**
     * Verify the validity of the chromosome. An exception should be thrown if the chromosome is invalid.
     * @param logger             Logger.
     * @param data               Thread-dependent data.
     */
    virtual void verify([[maybe_unused]] RLogger& logger, [[maybe_unused]] T& data) const {}

protected:

    /**
     * Set the value of a criterion (typically by evaluate). The number of criteria grows if necessary.
     * @param idx                Index of the criterion.
     * @param value              Value.
     */
    void criterion(const std::size_t idx, const double value)
    {
        if (idx >= criteria_.size())
            criteria_.resize(idx + 1, 0.0);
        criteria_[idx] = value;
    }

private:

    /**
     * Set the fitness of the chromosome (done by the population).
     * @param fitness            Fitness.
     */
    void fitness(const double fitness) { fitness_ = fitness; }

    //-----------------------------------------------------
    std::size_t id_ = 0;                    ///< Identifier.
    std::vector<double> criteria_;          ///< Values of the criteria.
    double fitness_ = 0.0;                  ///< Fitness.

    //-----------------------------------------------------
    template <class C> friend class RGAPopulation;
};


}  //------- End of namespace evocomb ----------------------------------------------
