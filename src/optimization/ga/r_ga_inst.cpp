/*

    EvoComb

    RGAInst.cpp

    Instance of Genetic Algorithms - Implementation.

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
// include files for ANSI C/C++
#include <limits>
#include <numeric>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "r_ga_inst.hpp"
#include "r_ga.hpp"
#include "r_ga_population.hpp"
using namespace evocomb;


//------------------------------------------------------------------------------
mLogModule("r.ga");


//------------------------------------------------------------------------------
namespace {

/**
 * Read an optional positive integer parameter.
 * @param ga                     "ga" object of the configuration.
 * @param key                    Key of the parameter.
 * @param value                  Value to set (unchanged if the parameter is missing).
 */
void readSize(const RJSON& ga, const std::string& key, std::size_t& value)
{
    const auto param = ga.find(key);
    if (param == nullptr)
        return;
    if (!param->isInteger() || param->toInteger() < 0)
        mThrowRGAException("'", key, "' must be a positive integer");
    value = static_cast<std::size_t>(param->toInteger());
}


/**
 * Read an optional boolean parameter.
 * @param ga                     "ga" object of the configuration.
 * @param key                    Key of the parameter.
 * @param value                  Value to set (unchanged if the parameter is missing).
 */
void readBool(const RJSON& ga, const std::string& key, bool& value)
{
    const auto param = ga.find(key);
    if (param == nullptr)
        return;
    if (!param->isBoolean())
        mThrowRGAException("'", key, "' must be a boolean");
    value = param->toBool();
}

}  //------- End of anonymous namespace -----------------------------------------



//------------------------------------------------------------------------------
//
// class RGAInst
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
RGAInst::RGAInst(RLogger& logger, const RJSON& config, RGAListener* listener):
    logger_(logger),
    listener_(listener),
    random_(seed_)
{
    mLogBlock(logger_, "RGAInst");
    applyConfig(config, true);
}


//------------------------------------------------------------------------------
RGAInst::~RGAInst() = default;


//------------------------------------------------------------------------------
void RGAInst::updateConfig(const std::string& json)
{
    mLogBlock(logger_, "updateConfig");
    applyConfig(RJSON::parse(json), false);
}


//------------------------------------------------------------------------------
void RGAInst::init(RGAGenericPopulation* population)
{
    mLogBlock(logger_, "init");
    if (population == nullptr)
        mThrowRGAException("Population is null");
    if (population->size(logger_) == 0)
        mThrowRGAException("Population is empty");
    if (nbMutations_ > population->size(logger_))
        mThrowRGAException("Number of mutations (", nbMutations_, ") cannot be greater than the population size (", population->size(logger_), ")");

    population_ = population;
    popSize_ = population_->size(logger_);
    population_->init(logger_);
    population_->createThreadData(logger_, nbThreads_);

    toEval_.assign(popSize_, 1);
    ranked_.resize(popSize_);
    std::iota(ranked_.begin(), ranked_.end(), 0);
    tmp1_.resize(popSize_);
    tmp2_.resize(popSize_);
    nbCross_ = popSize_ / 4;
    bestInPop_ = 0;
    hasBest_ = false;
    randomConstructed_ = false;
    gen_ = genLimit_ = ageBest_ = ageBestInPop_ = 0;
    ageNextMutation_ = freqMutation_;
    ageNextStrongMutation_ = freqStrongMutation_;
    ageNextInversion_ = freqInversion_;
}


//------------------------------------------------------------------------------
void RGAInst::run(const std::size_t nbGenerations)
{
    genLimit_ = gen_ + nbGenerations;
    run();
}


//------------------------------------------------------------------------------
std::size_t RGAInst::random(const std::size_t max)
{
    std::uniform_int_distribution<std::size_t> dist(0, max - 1);
    return dist(random_);
}


//------------------------------------------------------------------------------
void RGAInst::run()
{
    mLogBlock(logger_, "run");
    checkInit();

    if (!randomConstructed_)
    {
        randomConstruct();
        analyzePop();
        if (listener_ != nullptr)
            listener_->onGeneration(*this);
    }

    stopRequested_.store(false, std::memory_order_relaxed);
    while (!stopCondition() && !stopRequested_.load(std::memory_order_relaxed))
        generation();
    postRun();
}


//------------------------------------------------------------------------------
void RGAInst::analyzePop()
{
    mLogBlock(logger_, "analyzePop");

    // Evaluate the chromosomes, and modify (and evaluate again) the ones identical to better ones if necessary.
    evaluatePop();
    if (handleDuplicates_ && handleDuplicates())
        evaluatePop();

    // Rank the chromosomes and verify if the best chromosome of the population or the best one ever computed have
    // changed.
    std::iota(ranked_.begin(), ranked_.end(), 0);
    std::sort(ranked_.begin(), ranked_.end(), [this](const std::size_t a, const std::size_t b) {
        return isBetter(population_->fitness(logger_, a), population_->fitness(logger_, b));
    });
    if (const auto best = ranked_.front(); isBetter(population_->fitness(logger_, best), population_->fitness(logger_, bestInPop_)))
    {
        mLogDebug(logger_, "Chromosome ", best, " best of population");
        bestInPop_ = best;
        ageBestInPop_ = 0;
        ageNextMutation_ = freqMutation_;
    }
    if (!hasBest_ || isBetter(population_->fitness(logger_, bestInPop_), population_->bestFitness(logger_)))
    {
        mLogInfo(logger_, "Chromosome ", bestInPop_, " best ever (generation ", gen_, ")");
        population_->copyToBest(logger_, bestInPop_);
        hasBest_ = true;
        ageBest_ = 0;
        ageNextStrongMutation_ = freqStrongMutation_;
        if (listener_ != nullptr)
            listener_->onBest(*this);
    }
}


//------------------------------------------------------------------------------
void RGAInst::randomConstruct()
{
    mLogBlock(logger_, "randomConstruct");
    parallelRandom(popSize_, [this](const std::size_t thread, RGARandom& random, const std::size_t i) {
        population_->randomConstruct(logger_, thread, random, i);
        toEval_[i] = 1;
        if (verify_)
            population_->verify(logger_, thread, i);
    });
    randomConstructed_ = true;
}


//------------------------------------------------------------------------------
bool RGAInst::stopCondition()
{
    return gen_ >= genLimit_;
}


//------------------------------------------------------------------------------
void RGAInst::crossover()
{
    mLogBlock(logger_, "crossover");

    // At least 4 chromosomes are needed (2 parents and 2 children)
    if (nbCross_ == 0)
        return;

    // Apply a tournament: the losers are put at the end of tmp2_ (the first loser at the last position), the winner at
    // its beginning. So, tmp2_ contains the parents at its beginning and the children at its end.
    std::iota(tmp1_.begin(), tmp1_.end(), 0);
    auto nbToTreat = popSize_;
    auto nbTreated = popSize_ - 1;
    while (nbToTreat > 1)
    {
        // Choose two different chromosomes still in the tournament
        const auto idx1 = random(nbToTreat);
        auto idx2 = random(nbToTreat - 1);
        if (idx2 >= idx1)
            ++idx2;

        // The loser leaves the tournament
        const auto loser = isBetter(population_->fitness(logger_, tmp1_[idx1]), population_->fitness(logger_, tmp1_[idx2])) ? idx2 : idx1;
        tmp2_[nbTreated--] = tmp1_[loser];
        std::copy(tmp1_.begin() + static_cast<std::ptrdiff_t>(loser) + 1,
                  tmp1_.begin() + static_cast<std::ptrdiff_t>(nbToTreat),
                  tmp1_.begin() + static_cast<std::ptrdiff_t>(loser));
        --nbToTreat;
    }
    tmp2_[0] = tmp1_[0];

    // Plan the crossovers: each pair of parents produces two children (the children are different from the parents).
    tasks_.clear();
    parents_.clear();
    auto child = popSize_ - 1;
    for (std::size_t i = 0, parent = 0; i < nbCross_; ++i, parent += 2)
    {
        const auto p1 = tmp2_[parent];
        const auto p2 = tmp2_[parent + 1];

        // First crossover
        auto c = tmp2_[child--];
        mLogDebug(logger_, "Parent ", p2, " + Parent ", p1, " -> Child ", c);
        tasks_.push_back(c);
        parents_.push_back(p2);
        parents_.push_back(p1);

        // Second crossover
        c = tmp2_[child--];
        mLogDebug(logger_, "Parent ", p1, " + Parent ", p2, " -> Child ", c);
        tasks_.push_back(c);
        parents_.push_back(p1);
        parents_.push_back(p2);
    }

    // Make the crossovers in parallel
    parallelRandom(tasks_.size(), [this](const std::size_t thread, RGARandom& random, const std::size_t task) {
        const auto c = tasks_[task];
        population_->crossover(logger_, thread, random, parents_[2 * task], parents_[2 * task + 1], c);
        modified(thread, random, c);
    });
}


//------------------------------------------------------------------------------
void RGAInst::mutation()
{
    mLogBlock(logger_, "mutation");

    // The worst chromosomes become a copy of the best chromosome of the population. The copies are done before the
    // mutations since the best chromosome of the population may be one of the mutated chromosomes.
    tasks_.clear();
    for (std::size_t i = 0; i < nbMutations_; ++i)
    {
        const auto p = ranked_[popSize_ - 1 - i];
        population_->copy(logger_, p, bestInPop_);
        mLogDebug(logger_, "Mutation (best of population) -> Chromosome ", p);
        tasks_.push_back(p);
    }

    // Mutate them in parallel
    parallelRandom(tasks_.size(), [this](const std::size_t thread, RGARandom& random, const std::size_t task) {
        const auto p = tasks_[task];
        population_->mutation(logger_, thread, random, p);
        modified(thread, random, p);
    });
}


//------------------------------------------------------------------------------
void RGAInst::strongMutation()
{
    mLogBlock(logger_, "strongMutation");

    // The worst chromosomes become a strongly mutated copy of the best chromosome ever computed (which is not part of
    // the population, so the copies can be done in parallel).
    tasks_.clear();
    for (std::size_t i = 0; i < nbMutations_; ++i)
    {
        const auto p = ranked_[popSize_ - 1 - i];
        mLogDebug(logger_, "Strong mutation (best ever) -> Chromosome ", p);
        tasks_.push_back(p);
    }
    parallelRandom(tasks_.size(), [this](const std::size_t thread, RGARandom& random, const std::size_t task) {
        const auto p = tasks_[task];
        population_->copyFromBest(logger_, p);
        population_->strongMutation(logger_, thread, random, p);
        modified(thread, random, p);
    });
}


//------------------------------------------------------------------------------
void RGAInst::inversion()
{
    mLogBlock(logger_, "inversion");
    const auto p = random(popSize_);
    mLogDebug(logger_, "Inversion of Chromosome ", p);
    parallelRandom(1, [this, p](const std::size_t thread, RGARandom& random, std::size_t) {
        population_->inversion(logger_, thread, random, p);
        modified(thread, random, p);
    });
}


//------------------------------------------------------------------------------
void RGAInst::verify()
{
    mLogBlock(logger_, "verify");
    parallel(popSize_, [this](const std::size_t thread, const std::size_t i) {
        population_->verify(logger_, thread, i);
    });
    mLogDebug(logger_, "Instance Ok");
}


//------------------------------------------------------------------------------
void RGAInst::generation()
{
    mLogBlock(logger_, "generation");
    ++gen_;
    ++ageBest_;
    ++ageBestInPop_;
    mLogDebug(logger_, "Gen=", gen_, " ; AgeBest=", ageBest_, " ; AgeBestInPop=", ageBestInPop_);

    crossover();

    // For each operator enabled (non-null frequency), decrease the number of generations before it and verify if it
    // must be applied. An analysis is necessary before each one since the population was modified.
    if (freqMutation_ != 0 && --ageNextMutation_ == 0)
    {
        analyzePop();
        mutation();
        ageNextMutation_ = freqMutation_;
    }
    if (freqStrongMutation_ != 0 && --ageNextStrongMutation_ == 0)
    {
        analyzePop();
        strongMutation();
        ageNextStrongMutation_ = freqStrongMutation_;
    }
    if (freqInversion_ != 0 && --ageNextInversion_ == 0)
    {
        analyzePop();
        inversion();
        ageNextInversion_ = freqInversion_;
    }

    analyzePop();
    if (listener_ != nullptr)
        listener_->onGeneration(*this);
}


//------------------------------------------------------------------------------
void RGAInst::modified(const std::size_t thread, RGARandom& random, const std::size_t i)
{
    if (optimize_)
        population_->optimize(logger_, thread, random, i);
    toEval_[i] = 1;
    if (verify_)
        population_->verify(logger_, thread, i);
}


//------------------------------------------------------------------------------
void RGAInst::evaluatePop()
{
    // Evaluate (in parallel) all the chromosomes if necessary
    tasks_.clear();
    for (std::size_t i = 0; i < popSize_; ++i)
        if (toEval_[i])
            tasks_.push_back(i);
    if (!tasks_.empty())
    {
        parallel(tasks_.size(), [this](const std::size_t thread, const std::size_t task) {
            population_->evaluate(logger_, thread, tasks_[task]);
        });
        for (const auto i : tasks_)
            toEval_[i] = 0;
        doPostEvaluation_ = true;
    }

    // If necessary, compute the fitness (with the best chromosome ever computed, so that its fitness remains
    // comparable) and do a post-evaluation.
    if (doPostEvaluation_)
    {
        population_->computeFitness(logger_, *fitness_, hasBest_);
        postEvaluate();
        doPostEvaluation_ = false;
    }
}


//------------------------------------------------------------------------------
bool RGAInst::handleDuplicates()
{
    mLogBlock(logger_, "handleDuplicates");

    // Look (sequentially) for identical chromosomes: the worst one of each pair is modified with a probability of 90%.
    std::vector<char> toModify(popSize_, 0);
    for (std::size_t i = 0; i + 1 < popSize_; ++i)
        for (std::size_t j = i + 1; j < popSize_; ++j)
        {
            if (!population_->isSame(logger_, i, j))
                continue;
            const auto worst = isBetter(population_->fitness(logger_, i), population_->fitness(logger_, j)) ? j : i;
            if (random(100) < 90)
                toModify[worst] = 1;
        }
    tasks_.clear();
    for (std::size_t i = 0; i < popSize_; ++i)
        if (toModify[i])
        {
            mLogDebug(logger_, "Chromosome ", i, " is a duplicate: it is modified");
            tasks_.push_back(i);
        }
    if (tasks_.empty())
        return false;

    // Modify them in parallel
    parallelRandom(tasks_.size(), [this](const std::size_t thread, RGARandom& random, const std::size_t task) {
        const auto i = tasks_[task];
        population_->modify(logger_, thread, random, i);
        modified(thread, random, i);
    });
    return true;
}


//------------------------------------------------------------------------------
void RGAInst::parallelRandom(const std::size_t nbTasks, const std::function<void(std::size_t thread, RGARandom& random, std::size_t task)>& task)
{
    // Draw the seeds sequentially (the result does not depend on the thread executing a task).
    seeds_.resize(nbTasks);
    for (auto& seed : seeds_)
        seed = random_();

    pool_->run(nbTasks, [this, &task](const std::size_t thread, const std::size_t t) {
        auto& random = randoms_[thread];
        random.seed(seeds_[t]);
        task(thread, random, t);
    });
}


//------------------------------------------------------------------------------
void RGAInst::parallel(const std::size_t nbTasks, const std::function<void(std::size_t thread, std::size_t task)>& task) const
{
    pool_->run(nbTasks, task);
}


//------------------------------------------------------------------------------
void RGAInst::applyConfig(const RJSON& config, const bool initial)
{
    const auto ga = config.find("ga");
    if (ga == nullptr || !ga->isObject())
        mThrowRGAException("Missing 'ga' object");

    // Read all the parameters in local variables (initialized with the current values) and verify them before modifying
    // the instance.
    std::size_t seed = seed_;
    auto verify = verify_;
    auto optimize = optimize_;
    auto handleDuplicates = handleDuplicates_;
    auto maximize = maximize_;
    std::unique_ptr<RGAFitness> fitness;
    auto nbThreads = nbThreads_;
    auto nbMutations = nbMutations_;
    auto freqMutation = freqMutation_;
    auto freqStrongMutation = freqStrongMutation_;
    auto freqInversion = freqInversion_;
    readSize(*ga, "seed", seed);
    readSize(*ga, "nb_threads", nbThreads);
    readBool(*ga, "verify", verify);
    readBool(*ga, "optimize", optimize);
    readBool(*ga, "handle_duplicates", handleDuplicates);
    readBool(*ga, "maximize", maximize);
    if (const auto fitnessConfig = ga->find("fitness"); initial || fitnessConfig != nullptr)
    {
        if (!initial && population_ != nullptr)
            mThrowRGAException("'fitness' cannot be changed once the instance is initialized");
        fitness = RGAFitness::create(logger_, fitnessConfig);
    }
    if ((fitness ? fitness.get() : fitness_.get())->requiresMaximize() && !maximize)
        mThrowRGAException("The '", (fitness ? fitness.get() : fitness_.get())->name(), "' fitness must be maximized");
    readSize(*ga, "nb_mutations", nbMutations);
    readSize(*ga, "mutation_frequency", freqMutation);
    readSize(*ga, "strong_mutation_frequency", freqStrongMutation);
    readSize(*ga, "inversion_frequency", freqInversion);

    if (seed > std::numeric_limits<std::uint32_t>::max())
        mThrowRGAException("'seed' is too large");
    if (population_ != nullptr && maximize != maximize_)
        mThrowRGAException("'maximize' cannot be changed once the instance is initialized");
    if (nbThreads == 0)
        mThrowRGAException("'nb_threads' must be strictly positive");
    if (population_ != nullptr && nbMutations > popSize_)
        mThrowRGAException("Number of mutations (", nbMutations, ") cannot be greater than the population size (", popSize_, ")");

    // Create the threads (and the thread-dependent data) if their number changes. It is done first since it may fail.
    if (initial || nbThreads != nbThreads_)
    {
        auto pool = std::make_unique<RThreadPool>(nbThreads);
        if (population_ != nullptr)
            population_->createThreadData(logger_, nbThreads);
        pool_ = std::move(pool);
        randoms_.resize(nbThreads);
        nbThreads_ = nbThreads;
        mLogInfo(logger_, "GA uses ", nbThreads_, " thread(s)");
    }

    // Apply the parameters
    if (ga->find("seed") != nullptr)
    {
        seed_ = static_cast<std::uint32_t>(seed);
        random_.seed(seed_);
    }
    verify_ = verify;
    optimize_ = optimize;
    handleDuplicates_ = handleDuplicates;
    maximize_ = maximize;
    if (fitness)
        fitness_ = std::move(fitness);
    nbMutations_ = nbMutations;
    const auto applyFrequency = [&](const char* key, const char* op, const std::size_t value, std::size_t& freq, std::size_t& ageNext) {
        const auto present = ga->find(key) != nullptr;
        if (present)
            freq = ageNext = value;
        if (freq == 0 && (present || initial))
            mLogInfo(logger_, "'", key, "' is null: no ", op, " will be done");
    };
    applyFrequency("mutation_frequency", "mutation", freqMutation, freqMutation_, ageNextMutation_);
    applyFrequency("strong_mutation_frequency", "strong mutation", freqStrongMutation, freqStrongMutation_, ageNextStrongMutation_);
    applyFrequency("inversion_frequency", "inversion", freqInversion, freqInversion_, ageNextInversion_);
}


//------------------------------------------------------------------------------
void RGAInst::checkInit() const
{
    if (population_ == nullptr)
        mThrowRGAException("Instance not initialized");
}
