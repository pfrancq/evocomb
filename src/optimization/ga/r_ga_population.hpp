/*

    EvoComb

    RGAPopulation.hpp

    Population of the Genetic Algorithm - Header.

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
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "app/r_logger.hpp"
#include "r_ga.hpp"
#include "r_ga_chromo.hpp"
#include "r_ga_fitness.hpp"


//------------------------------------------------------------------------------
namespace evocomb {
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
 * The RGAGenericPopulation abstract class provides a representation for a population of chromosomes on which the GA
 * instance (RGAInst) can do some operations. The chromosomes are referenced by their index in the population. The
 * population also stores a copy of the best chromosome ever computed, and the thread-dependent data (one per thread).
 *
 * The operators that can be executed in parallel receive the index of the thread executing them (to select the
 * thread-dependent data) and the random generator to use.
 */
class RGAGenericPopulation
{
public:

    /**
     * Construct the population.
     */
    explicit RGAGenericPopulation() = default;

    /**
     * Destruct the population.
     */
    virtual ~RGAGenericPopulation() = default;

private:

    /**
     * Get the number of chromosomes of the population.
     * @param logger             Logger.
     */
    [[nodiscard]] virtual std::size_t size(RLogger& logger) const = 0;

    /**
     * Initialize the chromosomes (each one receives its identifier) and the best chromosome ever computed.
     * @param logger             Logger.
     */
    virtual void init(RLogger& logger) = 0;

    /**
     * Create the thread-dependent data (the previous ones are deleted).
     * @param logger             Logger.
     * @param nbThreads          Number of threads.
     */
    virtual void createThreadData(RLogger& logger, std::size_t nbThreads) = 0;

    /**
     * Construct randomly a chromosome.
     * @param logger             Logger.
     * @param thread             Index of the thread.
     * @param random             Random generator.
     * @param i                  Index of the chromosome.
     */
    virtual void randomConstruct(RLogger& logger, std::size_t thread, RGARandom& random, std::size_t i) = 0;

    /**
     * Do a crossover: the chromosome k becomes a child of the chromosomes i and j.
     * @param logger             Logger.
     * @param thread             Index of the thread.
     * @param random             Random generator.
     * @param i                  Index of the first parent.
     * @param j                  Index of the second parent.
     * @param k                  Index of the child (different from i and j).
     */
    virtual void crossover(RLogger& logger, std::size_t thread, RGARandom& random, std::size_t i, std::size_t j, std::size_t k) = 0;

    /**
     * Do a mutation of a chromosome.
     * @param logger             Logger.
     * @param thread             Index of the thread.
     * @param random             Random generator.
     * @param i                  Index of the chromosome.
     */
    virtual void mutation(RLogger& logger, std::size_t thread, RGARandom& random, std::size_t i) = 0;

    /**
     * Do a strong mutation of a chromosome.
     * @param logger             Logger.
     * @param thread             Index of the thread.
     * @param random             Random generator.
     * @param i                  Index of the chromosome.
     */
    virtual void strongMutation(RLogger& logger, std::size_t thread, RGARandom& random, std::size_t i) = 0;

    /**
     * Do an inversion of a chromosome.
     * @param logger             Logger.
     * @param thread             Index of the thread.
     * @param random             Random generator.
     * @param i                  Index of the chromosome.
     */
    virtual void inversion(RLogger& logger, std::size_t thread, RGARandom& random, std::size_t i) = 0;

    /**
     * Modify a chromosome identical to another one.
     * @param logger             Logger.
     * @param thread             Index of the thread.
     * @param random             Random generator.
     * @param i                  Index of the chromosome.
     */
    virtual void modify(RLogger& logger, std::size_t thread, RGARandom& random, std::size_t i) = 0;

    /**
     * Look if two chromosomes represent the same solution.
     * @param logger             Logger.
     * @param i                  Index of the first chromosome.
     * @param j                  Index of the second chromosome.
     */
    [[nodiscard]] virtual bool isSame(RLogger& logger, std::size_t i, std::size_t j) const = 0;

    /**
     * Perform a local optimization of a chromosome.
     * @param logger             Logger.
     * @param thread             Index of the thread.
     * @param random             Random generator.
     * @param i                  Index of the chromosome.
     */
    virtual void optimize(RLogger& logger, std::size_t thread, RGARandom& random, std::size_t i) = 0;

    /**
     * Compute the fitness of a chromosome.
     * @param logger             Logger.
     * @param thread             Index of the thread.
     * @param i                  Index of the chromosome.
     */
    virtual void evaluate(RLogger& logger, std::size_t thread, std::size_t i) = 0;

    /**
     * Verify the validity of a chromosome (an exception is thrown if it is invalid).
     * @param logger             Logger.
     * @param thread             Index of the thread.
     * @param i                  Index of the chromosome.
     */
    virtual void verify(RLogger& logger, std::size_t thread, std::size_t i) const = 0;

    /**
     * Compute the fitness of the chromosomes from the values of their criteria with a strategy (in a single call to the
     * strategy, so that the fitness values are comparable).
     * @param logger             Logger.
     * @param strategy           Fitness strategy.
     * @param withBest           Must the best chromosome ever computed be included (it is then the last solution)?
     */
    virtual void computeFitness(RLogger& logger, RGAFitness& strategy, bool withBest) = 0;

    /**
     * Get the fitness of a chromosome.
     * @param logger             Logger.
     * @param i                  Index of the chromosome.
     */
    [[nodiscard]] virtual double fitness(RLogger& logger, std::size_t i) const = 0;

    /**
     * Get the fitness of the best chromosome ever computed.
     * @param logger             Logger.
     */
    [[nodiscard]] virtual double bestFitness(RLogger& logger) const = 0;

    /**
     * Copy a chromosome into another one (the destination keeps its identifier).
     * @param logger             Logger.
     * @param dst                Index of the destination chromosome.
     * @param src                Index of the source chromosome.
     */
    virtual void copy(RLogger& logger, std::size_t dst, std::size_t src) = 0;

    /**
     * Copy a chromosome into the best chromosome ever computed (which keeps its identifier).
     * @param logger             Logger.
     * @param i                  Index of the chromosome.
     */
    virtual void copyToBest(RLogger& logger, std::size_t i) = 0;

    /**
     * Copy the best chromosome ever computed into a chromosome (which keeps its identifier).
     * @param logger             Logger.
     * @param i                  Index of the chromosome.
     */
    virtual void copyFromBest(RLogger& logger, std::size_t i) = 0;

    //-----------------------------------------------------
    friend class RGAInst;
};


//------------------------------------------------------------------------------
/**
 * The RGAPopulation template class implements a population of chromosomes. A class of chromosome must derive from
 * RGAChromo and provide a crossover method with the following signature:
 * @code
 * class Chromo : public RGAChromo<Data>
 * {
 * public:
 *      void crossover(RLogger& logger, Data& data, RGARandom& random, const Chromo& parent1, const Chromo& parent2);
 * };
 * @endcode
 *
 * The class of the thread-dependent data is given by the chromosome (C::ThreadData). By default, they are
 * default-constructed. If they need some parameters (for example the size of the problem), a derived class must
 * override newThreadData.
 * @tparam C                     Class of the chromosome.
 */
template <class C>
    class RGAPopulation : public RGAGenericPopulation
{
public:

    /**
     * Class of the thread-dependent data.
     */
    using ThreadData = typename C::ThreadData;

private:

    static_assert(std::is_base_of_v<RGAChromo<ThreadData>, C>, "C must inherit from RGAChromo");
    static_assert(std::is_default_constructible_v<C>, "C must be default-constructible");
    static_assert(std::is_copy_assignable_v<C>, "C must be copy-assignable");

public:

    /**
     * Construct the population.
     * @param nb                 Number of chromosomes.
     */
    explicit RGAPopulation(std::size_t nb)
    {
        population_.resize(nb);
    }

    /**
     * Get the number of chromosomes.
     */
    [[nodiscard]] std::size_t nbChromos() const { return population_.size(); }

    /**
     * Get a chromosome.
     * @param i                  Index of the chromosome.
     */
    [[nodiscard]] const C& operator[](std::size_t i) const { return population_[i]; }

    /**
     * Get the chromosomes.
     */
    [[nodiscard]] const std::vector<C>& chromos() const { return population_; }

    /**
     * Get the best chromosome ever computed (its identifier is the size of the population).
     */
    [[nodiscard]] const C& best() const { return best_; }

    /**
     * Get the number of thread-dependent data (the number of threads of the instance, 0 before its initialization).
     */
    [[nodiscard]] std::size_t nbThreadData() const { return threadData_.size(); }

    /**
     * Get the thread-dependent data of a thread.
     * @param thread             Index of the thread.
     */
    [[nodiscard]] const ThreadData& threadData(std::size_t thread) const { return *threadData_[thread]; }

protected:

    /**
     * Initialize a chromosome (called by init for each chromosome and for the best chromosome ever computed). By
     * default, C::init is called. A derived class can override it to give some data to the chromosomes.
     * @param logger             Logger.
     * @param chromo             Chromosome.
     * @param id                 Identifier of the chromosome.
     */
    virtual void initChromo(RLogger& logger, C& chromo, std::size_t id) { chromo.init(logger, id); }

    /**
     * Create the thread-dependent data of a thread. By default, it is default-constructed.
     * @param logger             Logger.
     * @param thread             Index of the thread.
     * @exception RGAException   ThreadData is not default-constructible (the method must then be overridden).
     */
    virtual std::unique_ptr<ThreadData> newThreadData([[maybe_unused]] RLogger& logger, [[maybe_unused]] std::size_t thread)
    {
        if constexpr (std::is_default_constructible_v<ThreadData>)
            return std::make_unique<ThreadData>();
        else
        {
            mLogModule("r.ga");
            mThrowRGAException("ThreadData is not default-constructible: newThreadData must be overridden");
        }
    }

private:

    /**
     * Get the number of chromosomes of the population.
     * @param logger             Logger.
     */
    [[nodiscard]] std::size_t size([[maybe_unused]] RLogger& logger) const override { return population_.size(); }

    /**
     * Initialize the chromosomes with the identifiers 0 to size()-1, and the best chromosome ever computed with the
     * identifier size().
     * @param logger             Logger.
     */
    void init(RLogger& logger) override
    {
        std::size_t id = 0;
        for (auto& chromo : population_)
            initChromo(logger, chromo, id++);
        initChromo(logger, best_, id);
    }

    /**
     * Create the thread-dependent data with newThreadData (the previous ones are deleted).
     * @param logger             Logger.
     * @param nbThreads          Number of threads.
     */
    void createThreadData(RLogger& logger, std::size_t nbThreads) override
    {
        threadData_.clear();
        threadData_.reserve(nbThreads);
        for (std::size_t thread = 0; thread < nbThreads; ++thread)
            threadData_.push_back(newThreadData(logger, thread));
    }

    /**
     * Call C::randomConstruct for a chromosome.
     * @param logger             Logger.
     * @param thread             Index of the thread.
     * @param random             Random generator.
     * @param i                  Index of the chromosome.
     */
    void randomConstruct(RLogger& logger, std::size_t thread, RGARandom& random, std::size_t i) override
    {
        population_[i].randomConstruct(logger, *threadData_[thread], random);
    }

    /**
     * Call C::crossover for the child with the two parents.
     * @param logger             Logger.
     * @param thread             Index of the thread.
     * @param random             Random generator.
     * @param i                  Index of the first parent.
     * @param j                  Index of the second parent.
     * @param k                  Index of the child (different from i and j).
     */
    void crossover(RLogger& logger, std::size_t thread, RGARandom& random, std::size_t i, std::size_t j, std::size_t k) override
    {
        const C& a = population_[i];
        const C& b = population_[j];
        population_[k].crossover(logger, *threadData_[thread], random, a, b);
    }

    /**
     * Call C::mutation for a chromosome.
     * @param logger             Logger.
     * @param thread             Index of the thread.
     * @param random             Random generator.
     * @param i                  Index of the chromosome.
     */
    void mutation(RLogger& logger, std::size_t thread, RGARandom& random, std::size_t i) override
    {
        population_[i].mutation(logger, *threadData_[thread], random);
    }

    /**
     * Call C::strongMutation for a chromosome.
     * @param logger             Logger.
     * @param thread             Index of the thread.
     * @param random             Random generator.
     * @param i                  Index of the chromosome.
     */
    void strongMutation(RLogger& logger, std::size_t thread, RGARandom& random, std::size_t i) override
    {
        population_[i].strongMutation(logger, *threadData_[thread], random);
    }

    /**
     * Call C::inversion for a chromosome.
     * @param logger             Logger.
     * @param thread             Index of the thread.
     * @param random             Random generator.
     * @param i                  Index of the chromosome.
     */
    void inversion(RLogger& logger, std::size_t thread, RGARandom& random, std::size_t i) override
    {
        population_[i].inversion(logger, *threadData_[thread], random);
    }

    /**
     * Call C::modify for a chromosome.
     * @param logger             Logger.
     * @param thread             Index of the thread.
     * @param random             Random generator.
     * @param i                  Index of the chromosome.
     */
    void modify(RLogger& logger, std::size_t thread, RGARandom& random, std::size_t i) override
    {
        population_[i].modify(logger, *threadData_[thread], random);
    }

    /**
     * Call C::isSame if C provides it (otherwise, the chromosomes are never identical).
     * @param logger             Logger.
     * @param i                  Index of the first chromosome.
     * @param j                  Index of the second chromosome.
     */
    [[nodiscard]] bool isSame([[maybe_unused]] RLogger& logger, std::size_t i, std::size_t j) const override
    {
        if constexpr (HasIsSame<C>::value)
            return population_[i].isSame(population_[j]);
        else
            return false;
    }

    /**
     * Detect if a class of chromosome provides isSame.
     */
    template<class X, class = void>
        struct HasIsSame : std::false_type {};
    template<class X>
        struct HasIsSame<X, std::void_t<decltype(std::declval<const X&>().isSame(std::declval<const X&>()))>> : std::true_type {};

    /**
     * Call C::optimize for a chromosome.
     * @param logger             Logger.
     * @param thread             Index of the thread.
     * @param random             Random generator.
     * @param i                  Index of the chromosome.
     */
    void optimize(RLogger& logger, std::size_t thread, RGARandom& random, std::size_t i) override
    {
        population_[i].optimize(logger, *threadData_[thread], random);
    }

    /**
     * Call C::evaluate for a chromosome.
     * @param logger             Logger.
     * @param thread             Index of the thread.
     * @param i                  Index of the chromosome.
     */
    void evaluate(RLogger& logger, std::size_t thread, std::size_t i) override
    {
        population_[i].evaluate(logger, *threadData_[thread]);
    }

    /**
     * Call C::verify for a chromosome.
     * @param logger             Logger.
     * @param thread             Index of the thread.
     * @param i                  Index of the chromosome.
     */
    void verify(RLogger& logger, std::size_t thread, std::size_t i) const override
    {
        population_[i].verify(logger, *threadData_[thread]);
    }

    /**
     * Compute the fitness of the chromosomes (and of the best chromosome ever computed if asked) with a strategy.
     * @param logger             Logger.
     * @param strategy           Fitness strategy.
     * @param withBest           Must the best chromosome ever computed be included (it is then the last solution)?
     */
    void computeFitness(RLogger& logger, RGAFitness& strategy, const bool withBest) override
    {
        criteria_.clear();
        for (const auto& chromo : population_)
            criteria_.push_back(&chromo.criteria());
        if (withBest)
            criteria_.push_back(&best_.criteria());
        strategy.compute(logger, criteria_, fitness_);
        for (std::size_t i = 0; i < population_.size(); ++i)
            population_[i].fitness(fitness_[i]);
        if (withBest)
            best_.fitness(fitness_.back());
    }

    /**
     * Get the fitness of a chromosome.
     * @param logger             Logger.
     * @param i                  Index of the chromosome.
     */
    [[nodiscard]] double fitness([[maybe_unused]] RLogger& logger, std::size_t i) const override { return population_[i].fitness(); }

    /**
     * Get the fitness of the best chromosome ever computed.
     * @param logger             Logger.
     */
    [[nodiscard]] double bestFitness([[maybe_unused]] RLogger& logger) const override { return best_.fitness(); }

    /**
     * Copy a chromosome into another one (the destination keeps its identifier). Nothing is done if both are the same
     * chromosome.
     * @param logger             Logger.
     * @param dst                Index of the destination chromosome.
     * @param src                Index of the source chromosome.
     */
    void copy([[maybe_unused]] RLogger& logger, std::size_t dst, std::size_t src) override
    {
        if (dst == src)
            return;
        assign(population_[dst], population_[src]);
    }

    /**
     * Copy a chromosome into the best chromosome ever computed (which keeps its identifier).
     * @param logger             Logger.
     * @param i                  Index of the chromosome.
     */
    void copyToBest([[maybe_unused]] RLogger& logger, std::size_t i) override { assign(best_, population_[i]); }

    /**
     * Copy the best chromosome ever computed into a chromosome (which keeps its identifier).
     * @param logger             Logger.
     * @param i                  Index of the chromosome.
     */
    void copyFromBest([[maybe_unused]] RLogger& logger, std::size_t i) override { assign(population_[i], best_); }

    /**
     * Copy a chromosome into another one while keeping the identifier of the destination.
     * @param dst                Destination chromosome.
     * @param src                Source chromosome.
     */
    static void assign(C& dst, const C& src)
    {
        const auto id = dst.id_;
        dst = src;
        dst.id_ = id;
    }

    //-----------------------------------------------------
    std::vector<C> population_;             ///< Chromosomes.
    C best_;                                ///< Best chromosome ever computed.
    std::vector<std::unique_ptr<ThreadData>> threadData_; ///< Thread-dependent data (one per thread).
    std::vector<const std::vector<double>*> criteria_;   ///< Criteria passed to the fitness strategy.
    std::vector<double> fitness_;           ///< Fitness computed by the fitness strategy.
};


}  //------- End of namespace evocomb ----------------------------------------------
