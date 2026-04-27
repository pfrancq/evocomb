/*

    EvoComb

    RGAInst.hpp

    Instance of Genetic Algorithms - Header.

    Copyright 1998-2026 by Pascal Francq (pascal@francq.info).
    Copyright 1998-2008 by the Université Libre de Bruxelles (ULB).

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
#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <random>
#include <string>
#include <vector>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "app/r_logger.hpp"
#include "tools/r_json.hpp"
#include "tools/r_thread_pool.hpp"
#include "r_ga.hpp"
#include "r_ga_fitness.hpp"


//------------------------------------------------------------------------------
namespace evocomb {
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// forward declaration
class RGAGenericPopulation;
class RGAInst;


//------------------------------------------------------------------------------
/**
 * The RGAListener class provides a way to be notified by a GA instance of its progress. All the methods do nothing by
 * default, so that a listener only overrides the notifications it needs:
 * @code
 * class MyListener : public evocomb::RGAListener
 * {
 * public:
 *     void onBest(const evocomb::RGAInst& inst) override
 *     {
 *         std::cout << "New best at generation " << inst.gen() << std::endl;
 *     }
 * };
 *
 * MyListener listener;
 * evocomb::RGAInst inst(logger, config, &listener);
 * @endcode
 * @short GA Listener.
 */
class RGAListener
{
public:

    RGAListener() = default;
    virtual ~RGAListener() = default;

    /**
     * Called when the best chromosome ever computed changes.
     * @param inst               Instance (inst.gen() gives the current generation).
     */
    virtual void onBest([[maybe_unused]] const RGAInst& inst) {}

    /**
     * Called at the end of each generation (and after the random construction).
     * @param inst               Instance (inst.gen() gives the current generation).
     */
    virtual void onGeneration([[maybe_unused]] const RGAInst& inst) {}
};


//------------------------------------------------------------------------------
/**
 * The RGAInst class provides an instance of the generic GA. It works on a population (see RGAPopulation) which it does
 * not own.
 *
 * Each generation does:
 * - A crossover (tournament strategy) that replaces the losers of the
 *   tournament by children of the winners.
 * - Periodically, a mutation of the worst chromosomes (they become a mutated
 *   copy of the best chromosome of the population).
 * - Periodically, a strong mutation of the worst chromosomes (they become a
 *   strongly mutated copy of the best chromosome ever computed).
 * - Periodically, an inversion of a random chromosome.
 *
 * The frequencies of the mutation, the strong mutation and the inversion are 0 by default, which means that the
 * corresponding operator is never applied.
 *
 * The parameters are read from the "ga" object of the configuration (all of them are optional):
 * @code
 * {
 *   "ga": {
 *     "seed": 12345,                     // Seed of the random generator.
 *     "nb_threads": 1,                   // Number of threads executing the operators.
 *     "verify": false,                   // Verify the chromosomes after each operation.
 *     "maximize": true,                  // Maximize (true) or minimize (false) the fitness.
 *     "fitness": {"type": "direct"},     // Fitness strategy (see below).
 *     "optimize": true,                  // Call RGAChromo::optimize after each operation.
 *     "handle_duplicates": false,        // Modify the chromosomes identical to better ones.
 *     "nb_mutations": 1,                 // Number of chromosomes mutated.
 *     "mutation_frequency": 0,           // Generations between two mutations (0 = never).
 *     "strong_mutation_frequency": 0,    // Generations between two strong mutations (0 = never).
 *     "inversion_frequency": 0           // Generations between two inversions (0 = never).
 *   }
 * }
 * @endcode
 *
 * The chromosomes compute the values of their criteria, and the fitness is computed from these values by a strategy
 * (see RGAFitness), selected by the "fitness" object:
 * - {"type": "direct"} (default): the fitness is the first criterion.
 * - {"type": "promethee", "criteria": [...]}: the fitness is the Fi computed
 *   by PROMETHEE II over the population and the best chromosome ever
 *   computed (the criteria are described as in RPromKernel::addCriteria).
 *   The fitness must then be maximized.
 *
 * If "handle_duplicates" is true, each analysis of the population looks for identical chromosomes (see RGAChromo). For
 * each pair, the worst one is modified (RGAChromo::modify) with a probability of 90%, and evaluated again. It avoids
 * that the population converges to copies of a single solution.
 *
 * The random generator is always seeded with the same value by default, so that the GA produces the same result for the
 * same problem (useful for debugging).
 *
 * The operators (random construction, crossover, mutations, inversion, optimization, evaluation and verification) are
 * executed in parallel by nb_threads threads. Each phase (for example all the crossovers of a generation) is first
 * planned sequentially (choice of the chromosomes and of a random seed for each operation), then executed in parallel.
 * The random generator passed to an operator is seeded with the seed of its operation, so that the result of the GA
 * does not depend on the number of threads. The phases themselves are done one after the other.
 *
 * A derived class can adapt the GA by overriding the virtual methods (for example stopCondition or postEvaluate). To
 * follow the progress of the GA, a listener (see RGAListener) can be passed to the instance.
 * @short Generic GA instance.
 */
class RGAInst
{
public:

    /**
     * Construct the instance.
     * @param logger             Logger.
     * @param config             Configuration (must contain a "ga" object).
     * @param listener           Listener notified of the progress of the GA (not owned, may be null).
     * @exception RGAException   The configuration is invalid.
     */
    RGAInst(RLogger& logger, const RJSON& config, RGAListener* listener = nullptr);

    /**
     * Destruct the instance.
     */
    virtual ~RGAInst();

    /**
     * Update the configuration of the instance. The JSON string has the same format as the configuration passed to the
     * constructor (a "ga" object). Only the parameters present are modified; the others keep their current values:
     * @code
     * inst.updateConfig(R"({"ga": {"nb_mutations": 3, "verify": true}})");
     * @endcode
     * If the seed is present, the random generator is reset. If a frequency is present, the corresponding counter is
     * restarted (a null frequency disables the operator, which is logged). "maximize" and "fitness" cannot be changed
     * once the instance is initialized. The update is atomic: if a parameter is invalid, nothing is modified.
     * @param json               JSON string.
     * @exception RGAException   The configuration is invalid.
     */
    void updateConfig(const std::string& json);

    /**
     * Look if the chromosomes are verified during the run.
     */
    [[nodiscard]] bool mustVerify() const { return verify_; }

    /**
     * Look if the fitness is maximized (true) or minimized (false).
     */
    [[nodiscard]] bool mustMaximize() const { return maximize_; }

    /**
     * Get the fitness strategy.
     */
    [[nodiscard]] const RGAFitness& fitnessStrategy() const { return *fitness_; }

    /**
     * Compare two fitness values.
     * @param fitness1           First fitness.
     * @param fitness2           Second fitness.
     * @return `true` if fitness1 is strictly better than fitness2 (greater if the fitness is maximized, lower if it is
     * minimized).
     */
    [[nodiscard]] bool isBetter(const double fitness1, const double fitness2) const
    {
        return maximize_ ? fitness1 > fitness2 : fitness1 < fitness2;
    }

    /**
     * Look if the identical chromosomes are handled.
     */
    [[nodiscard]] bool mustHandleDuplicates() const { return handleDuplicates_; }

    /**
     * Look if the chromosomes are optimized after each operation.
     */
    [[nodiscard]] bool mustOptimize() const { return optimize_; }

    /**
     * Get the number of chromosomes mutated by each mutation operation.
     */
    [[nodiscard]] std::size_t nbMutations() const { return nbMutations_; }

    /**
     * Get the number of generations between two mutations (0 if they are never done).
     */
    [[nodiscard]] std::size_t mutationFrequency() const { return freqMutation_; }

    /**
     * Get the number of generations between two strong mutations (0 if they are never done).
     */
    [[nodiscard]] std::size_t strongMutationFrequency() const { return freqStrongMutation_; }

    /**
     * Get the number of generations between two inversions (0 if they are never done).
     */
    [[nodiscard]] std::size_t inversionFrequency() const { return freqInversion_; }

    /**
     * Get the number of threads executing the operators.
     */
    [[nodiscard]] std::size_t nbThreads() const { return nbThreads_; }

    /**
     * Get the seed of the internal random generator.
     */
    [[nodiscard]] std::uint32_t seed() const { return seed_; }

    /**
     * Initialize the instance with a population (which is initialized).
     * @param population         Population (not owned by the instance, it must live as long as the instance uses it).
     * @exception RGAException   The population is null or the parameters are incompatible with its size.
     */
    virtual void init(RGAGenericPopulation* population);

    /**
     * Run the GA for a given number of generations (or until stop is called). It sets the limit used by the default
     * stopCondition.
     * @param nbGenerations      Number of generations to do.
     */
    void run(std::size_t nbGenerations);

    /**
     * Stop the GA at the end of the current generation (postRun is then called). It can be called from another thread
     * or from a listener. A call done before run is ignored (run resets the request).
     */
    void stop() { stopRequested_.store(true, std::memory_order_relaxed); }

    /**
     * Get the size of the population (0 if the instance is not initialized).
     */
    [[nodiscard]] std::size_t popSize() const { return popSize_; }

    /**
     * Get the number of generations done.
     */
    [[nodiscard]] std::size_t gen() const { return gen_; }

    /**
     * Get the age of the best chromosome ever computed.
     */
    [[nodiscard]] std::size_t ageBest() const { return ageBest_; }

    /**
     * Get the age of the best chromosome of the population.
     */
    [[nodiscard]] std::size_t ageBestInPop() const { return ageBestInPop_; }

    /**
     * Get the index of the best chromosome of the population.
     */
    [[nodiscard]] std::size_t bestInPop() const { return bestInPop_; }

    /**
     * Get the indexes of the chromosomes ranked from the best to the worst (as computed by the last analysis of the
     * population).
     */
    [[nodiscard]] const std::vector<std::size_t>& ranked() const { return ranked_; }

    /**
     * Get a random number in [0, max[.
     * @param max                Upper bound (must be strictly positive).
     */
    [[nodiscard]] std::size_t random(std::size_t max);

    /**
     * Randomly reorder a sequence using the internal random generator.
     * @param first              First element.
     * @param last               Last element (excluded).
     */
    template<class It>
        void randomOrder(It first, It last) { std::shuffle(first, last, random_); }

    //-----------------------------------------------------
    RGAInst(const RGAInst&) = delete;
    RGAInst& operator=(const RGAInst&) = delete;

protected:

    /**
     * Run the GA until stopCondition returns `true` or stop is called. The first run constructs randomly the
     * chromosomes.
     * @exception RGAException   The instance is not initialized.
     */
    virtual void run();

    /**
     * Analyze the population: evaluate the modified chromosomes, rank them, and update the best chromosome of the
     * population and the best one ever computed.
     */
    virtual void analyzePop();

    /**
     * Do a post-evaluation treatment. It is called by analyzePop when at least one chromosome was evaluated, once the
     * fitness is computed.
     */
    virtual void postEvaluate() {}

    /**
     * Construct randomly the chromosomes.
     */
    virtual void randomConstruct();

    /**
     * Determine if the GA must stop. It is called before each generation. By default, it returns `true` when the limit
     * set by run(nbGenerations) is reached.
     */
    [[nodiscard]] virtual bool stopCondition();

    /**
     * Called after the GA stops.
     */
    virtual void postRun() {}

    /**
     * Do the crossovers of a generation (tournament strategy).
     */
    virtual void crossover();

    /**
     * Do the mutations of a generation.
     */
    virtual void mutation();

    /**
     * Do the strong mutations of a generation.
     */
    virtual void strongMutation();

    /**
     * Do the inversion of a generation.
     */
    virtual void inversion();

    /**
     * Verify all the chromosomes.
     */
    virtual void verify();

    /**
     * Get the logger.
     */
    [[nodiscard]] RLogger& logger() const { return logger_; }

private:

    /**
     * Do a generation.
     */
    void generation();

    /**
     * Treat a chromosome modified by an operator: it is optimized (if necessary), marked to be evaluated and verified
     * (if necessary).
     * @param thread             Index of the thread.
     * @param random             Random generator.
     * @param i                  Index of the chromosome.
     */
    void modified(std::size_t thread, RGARandom& random, std::size_t i);

    /**
     * Evaluate (in parallel) the chromosomes to evaluate and, if at least one was evaluated, compute the fitness and do
     * the post-evaluation.
     */
    void evaluatePop();

    /**
     * Modify (in parallel) the chromosomes identical to better ones (with a probability of 90%).
     * @return `true` if at least one chromosome was modified.
     */
    bool handleDuplicates();

    /**
     * Execute tasks in parallel. Before, a seed is drawn (sequentially) for each task with the random generator of the
     * instance: the random generator passed to a task is seeded with it.
     * @param nbTasks            Number of tasks.
     * @param task               Function executing a task. Its parameters are the index of the thread, the random
     *                           generator and the index of the task.
     */
    void parallelRandom(std::size_t nbTasks, const std::function<void(std::size_t thread, RGARandom& random, std::size_t task)>& task);

    /**
     * Execute tasks (not needing random numbers) in parallel.
     * @param nbTasks            Number of tasks.
     * @param task               Function executing a task. Its parameters are the index of the thread and the index of
     *                           the task.
     */
    void parallel(std::size_t nbTasks, const std::function<void(std::size_t thread, std::size_t task)>& task) const;

    /**
     * Apply a configuration (see updateConfig). A message is logged for each null frequency present in the
     * configuration. For the initial configuration, it is also logged for each missing frequency (the operator is then
     * disabled by default).
     * @param config             Configuration (must contain a "ga" object).
     * @param initial            Is it the initial configuration (called by the constructor)?
     * @exception RGAException   The configuration is invalid.
     */
    void applyConfig(const RJSON& config, bool initial);

    /**
     * Throw an exception if the instance is not initialized.
     */
    void checkInit() const;

    //-----------------------------------------------------
    RLogger& logger_;                                   ///< Logger.
    RGAListener* listener_ = nullptr;                   ///< Listener (not owned, may be null).
    RGAGenericPopulation* population_ = nullptr;        ///< Population (not owned).
    std::size_t popSize_ = 0;                           ///< Size of the population.
    std::uint32_t seed_ = 12345;                        ///< Seed of the random generator.
    RGARandom random_;                                  ///< Random generator (used sequentially).
    std::size_t nbThreads_ = 1;                         ///< Number of threads.
    std::unique_ptr<RThreadPool> pool_;                 ///< Threads executing the operators.
    std::vector<RGARandom> randoms_;                    ///< Random generator of each thread.
    std::vector<std::uint32_t> seeds_;                  ///< Seeds of the tasks being executed.
    std::vector<std::size_t> tasks_;                    ///< Chromosomes concerned by the tasks being executed.
    std::vector<std::size_t> parents_;                  ///< Parents of the crossovers (two per crossover).
    bool verify_ = false;                               ///< Verify the chromosomes?
    bool optimize_ = true;                              ///< Optimize the chromosomes?
    bool handleDuplicates_ = false;                     ///< Handle the identical chromosomes?
    bool maximize_ = true;                              ///< Maximize (or minimize) the fitness?
    std::unique_ptr<RGAFitness> fitness_;               ///< Fitness strategy.
    bool randomConstructed_ = false;                    ///< Random construction done?
    bool hasBest_ = false;                              ///< Best chromosome ever computed set?
    bool doPostEvaluation_ = false;                     ///< Post-evaluation needed?
    std::atomic<bool> stopRequested_{false};         ///< Stop requested?
    std::vector<char> toEval_;                          ///< Chromosomes to evaluate (char and not bool: written in parallel).
    std::vector<std::size_t> ranked_;                   ///< Chromosomes ranked by fitness.
    std::vector<std::size_t> tmp1_;                     ///< Temporary indexes (tournament).
    std::vector<std::size_t> tmp2_;                     ///< Temporary indexes (tournament).
    std::size_t bestInPop_ = 0;                         ///< Best chromosome of the population.
    std::size_t gen_ = 0;                               ///< Number of generations done.
    std::size_t genLimit_ = 0;                          ///< Limit used by stopCondition.
    std::size_t ageBest_ = 0;                           ///< Age of the best chromosome ever.
    std::size_t ageBestInPop_ = 0;                      ///< Age of the best chromosome of the population.
    std::size_t nbCross_ = 0;                           ///< Number of crossover pairs per generation.
    std::size_t nbMutations_ = 1;                       ///< Number of chromosomes mutated.
    std::size_t freqMutation_ = 0;                      ///< Generations between two mutations (0 = never).
    std::size_t freqStrongMutation_ = 0;                ///< Generations between two strong mutations (0 = never).
    std::size_t freqInversion_ = 0;                     ///< Generations between two inversions (0 = never).
    std::size_t ageNextMutation_ = 0;                   ///< Generations left before the next mutation.
    std::size_t ageNextStrongMutation_ = 0;             ///< Generations left before the next strong mutation.
    std::size_t ageNextInversion_ = 0;                  ///< Generations left before the next inversion.
};


}  //------- End of namespace evocomb ----------------------------------------------
