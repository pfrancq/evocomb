/*

EvoComb

    RGABasicTests.cpp

    Copyright 2013-2026 by Pascal Francq (pascal@francq.info).

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
// include files for ANSI C/C++
#include <algorithm>
#include <array>
#include <bitset>
#include <cstdint>
#include <numeric>
#include <random>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "r_google_test.hpp"
#include "ga/r_ga.hpp"
#include "ga/r_ga_inst.hpp"
#include "ga/r_ga_population.hpp"
using namespace evocomb;


//------------------------------------------------------------------------------
mLogModule("r.unit_tests");


//------------------------------------------------------------------------------
/**
 * Thread-dependent data of DummyChromo: it only counts the operations done by each thread.
 */
struct DummyThreadData
{
    std::size_t nbOperations = 0;
};


//------------------------------------------------------------------------------
class DummyChromo : public RGAChromo<DummyThreadData>
{
public:

    //-----------------------------------------------------
    static constexpr std::size_t Size = 32;

    void randomConstruct(RLogger& logger, DummyThreadData& data, RGARandom& random) override
    {
        mLogBlock(logger, "randomConstruct");
        ++data.nbOperations;
        std::bernoulli_distribution dist(0.5);
        for (std::size_t i = 0; i < Size; ++i)
            adn_[i] = dist(random);
    }

    void mutation(RLogger& logger, DummyThreadData& data, RGARandom& random) override
    {
        mLogBlock(logger, "mutation");
        ++data.nbOperations;

        // Take 2 ou 3 positions and invert their bits
        std::uniform_int_distribution<std::size_t> nb_dist(2, 3);
        const auto nb = nb_dist(random);
        std::array<std::size_t, Size> positions;
        std::iota(positions.begin(), positions.end(), 0);
        std::array<std::size_t, 3> chosen;
        const auto last = std::sample(positions.begin(), positions.end(), chosen.begin(), nb, random);
        for (auto it = chosen.begin(); it != last; ++it)
            adn_.flip(*it);
    }

    void inversion(RLogger& logger, DummyThreadData& data, RGARandom& random) override
    {
        mLogBlock(logger, "inversion");
        ++data.nbOperations;

        // Choose a sequence [a, b] and invert their bits
        std::uniform_int_distribution<std::size_t> pos_dist(0, Size - 1);
        auto a = pos_dist(random);
        auto b = pos_dist(random);
        if (a > b)
            std::swap(a, b);
        for (auto i = a; i <= b; ++i)
            adn_.flip(i);
    }

    void crossover(RLogger& logger, DummyThreadData& data, RGARandom& random, const DummyChromo& parent1, const DummyChromo& parent2)
    {
        mLogBlock(logger, "crossover");
        ++data.nbOperations;

        // Bits [0, pos) from parent1, bits [pos, Size) from parent2
        std::uniform_int_distribution<std::size_t> pos_dist(1, Size - 1);
        const auto pos = pos_dist(random);
        const auto mask = std::bitset<Size>{}.set() << pos;
        adn_ = (parent1.adn_ & ~mask) | (parent2.adn_ & mask);
    }

    void evaluate(RLogger& logger, DummyThreadData& data) override
    {
        mLogBlock(logger, "evaluate");
        ++data.nbOperations;
        // Criterion 0: value of the bits ; criterion 1: number of bits set
        criterion(0, static_cast<double>(adn_.to_ulong()));
        criterion(1, static_cast<double>(adn_.count()));
    }

private:

    //-----------------------------------------------------
    std::bitset<Size> adn_;
};


//------------------------------------------------------------------------------
using RGADummyPopulation = RGAPopulation<DummyChromo>;


//------------------------------------------------------------------------------
class CountListener : public RGAListener
{
public:
    explicit CountListener(RGADummyPopulation& population): population_(population) {}

    void onBest(const RGAInst& inst) override
    {
        ++nbBest;
        lastBestGen = inst.gen();
        const auto fitness = population_[inst.bestInPop()].fitness();
        assert(fitness > bestFitness_);
        bestFitness_ = fitness;
    }

    void onGeneration(const RGAInst& inst) override
    {
        EXPECT_EQ(inst.gen(), nbGenerations);
        ++nbGenerations;
    }

    //-----------------------------------------------------
    RGADummyPopulation& population_;
    std::size_t nbBest = 0;
    std::size_t lastBestGen = 0;
    std::size_t nbGenerations = 0;
    double bestFitness_ = 0.0;
};


//------------------------------------------------------------------------------
class RGABasicTests : public RGoogleTest
{
};


//------------------------------------------------------------------------------
TEST_F(RGABasicTests, test)
{
    RGADummyPopulation population(16);
    CountListener listener(population);
    const auto config = RJSON::parse(R"({"ga": {"nb_mutations": 2, "verify": false, "mutation_frequency": 5, "inversion_frequency": 10}})");
    RGAInst inst(logger, config, &listener);
    inst.init(&population);
    inst.run(20);
    EXPECT_EQ(inst.gen(), 20u);

    // The best chromosome ever computed is at least as good as all the chromosomes of the population, and has its own
    // identifier.
    const auto& best = population.best();
    EXPECT_EQ(best.id(), population.nbChromos());
    for (const auto& chromo : population.chromos())
        EXPECT_GE(best.fitness(), chromo.fitness());

    // The identifiers are not modified by the copies.
    for (std::size_t i = 0; i < population.nbChromos(); ++i)
        EXPECT_EQ(population[i].id(), i);

    // The ranking is consistent with the fitness.
    const auto& ranked = inst.ranked();
    ASSERT_EQ(ranked.size(), population.nbChromos());
    for (std::size_t i = 1; i < ranked.size(); ++i)
        EXPECT_GE(population[ranked[i - 1]].fitness(), population[ranked[i]].fitness());

    // Running again continues from the current generation.
    inst.run(5);
    EXPECT_EQ(inst.gen(), 25u);
}


//------------------------------------------------------------------------------
TEST_F(RGABasicTests, config)
{
    EXPECT_THROW(RGAInst(logger, RJSON::parse(R"({})")), RGAException);
    EXPECT_THROW(RGAInst(logger, RJSON::parse(R"({"ga": {"verify": 1}})")), RGAException);

    // More mutations than chromosomes
    RGAPopulation<DummyChromo> population(4);
    RGAInst inst(logger, RJSON::parse(R"({"ga": {"nb_mutations": 5}})"));
    EXPECT_THROW(inst.init(&population), RGAException);

    // Not initialized
    RGAInst inst2(logger, RJSON::parse(R"({"ga": {}})"));
    EXPECT_THROW(inst2.run(1), RGAException);
}


//------------------------------------------------------------------------------
TEST_F(RGABasicTests, updateConfig)
{
    RGAInst inst(logger, RJSON::parse(R"({"ga": {"seed": 7, "nb_mutations": 2}})"));
    EXPECT_EQ(inst.seed(), 7u);
    EXPECT_EQ(inst.nbMutations(), 2u);
    EXPECT_FALSE(inst.mustVerify());

    // The frequencies are null by default
    EXPECT_EQ(inst.mutationFrequency(), 0u);
    EXPECT_EQ(inst.strongMutationFrequency(), 0u);
    EXPECT_EQ(inst.inversionFrequency(), 0u);

    // Only the parameters present are modified
    inst.updateConfig(R"({"ga": {"verify": true, "mutation_frequency": 3}})");
    EXPECT_TRUE(inst.mustVerify());
    EXPECT_EQ(inst.mutationFrequency(), 3u);
    EXPECT_EQ(inst.seed(), 7u);
    EXPECT_EQ(inst.nbMutations(), 2u);
    EXPECT_EQ(inst.inversionFrequency(), 0u);

    // A frequency can be set back to 0
    inst.updateConfig(R"({"ga": {"mutation_frequency": 0}})");
    EXPECT_EQ(inst.mutationFrequency(), 0u);

    // An invalid configuration modifies nothing
    EXPECT_THROW(inst.updateConfig(R"({"ga": {"optimize": false, "inversion_frequency": -1}})"), RGAException);
    EXPECT_TRUE(inst.mustOptimize());
    EXPECT_EQ(inst.inversionFrequency(), 0u);

    // Number of mutations checked against the population size once initialized
    RGAPopulation<DummyChromo> population(4);
    inst.init(&population);
    EXPECT_THROW(inst.updateConfig(R"({"ga": {"nb_mutations": 5}})"), RGAException);
    EXPECT_EQ(inst.nbMutations(), 2u);
    inst.updateConfig(R"({"ga": {"nb_mutations": 4}})");
    EXPECT_EQ(inst.nbMutations(), 4u);
    inst.run(10);
    EXPECT_EQ(inst.gen(), 10u);
}


//------------------------------------------------------------------------------
TEST_F(RGABasicTests, threads)
{
    // Run the same GA with 1 and 4 threads
    const auto runGA = [this](const std::size_t nbThreads) {
        auto population = std::make_unique<RGADummyPopulation>(32);
        RGAInst inst(logger, RJSON::parse(R"({"ga": {"nb_threads": )" + std::to_string(nbThreads) +
            R"(, "nb_mutations": 2, "mutation_frequency": 3, "strong_mutation_frequency": 5, "inversion_frequency": 4, "verify": true}})"));
        EXPECT_EQ(inst.nbThreads(), nbThreads);
        inst.init(population.get());
        inst.run(30);

        // One thread-dependent data per thread, and all the operations were done with them.
        EXPECT_EQ(population->nbThreadData(), nbThreads);
        std::size_t nbOperations = 0;
        for (std::size_t thread = 0; thread < population->nbThreadData(); ++thread)
            nbOperations += population->threadData(thread).nbOperations;
        EXPECT_GT(nbOperations, population->nbChromos());
        return population;
    };
    const auto pop1 = runGA(1);
    const auto pop4 = runGA(4);

    // The result does not depend on the number of threads
    for (std::size_t i = 0; i < pop1->nbChromos(); ++i)
        EXPECT_EQ((*pop1)[i].fitness(), (*pop4)[i].fitness());
    EXPECT_EQ(pop1->best().fitness(), pop4->best().fitness());

    // The number of threads can be changed after the initialization
    RGADummyPopulation population(16);
    RGAInst inst(logger, RJSON::parse(R"({"ga": {"mutation_frequency": 2}})"));
    inst.init(&population);
    inst.run(5);
    inst.updateConfig(R"({"ga": {"nb_threads": 3}})");
    EXPECT_EQ(population.nbThreadData(), 3u);
    inst.run(5);
    EXPECT_EQ(inst.gen(), 10u);
    EXPECT_THROW(inst.updateConfig(R"({"ga": {"nb_threads": 0}})"), RGAException);
}


//------------------------------------------------------------------------------
TEST_F(RGABasicTests, stop)
{
    // Listener stopping the GA after 3 generations
    struct StopListener : public RGAListener
    {
        RGAInst* inst = nullptr;
        void onGeneration(const RGAInst& i) override
        {
            if (i.gen() == 3)
                inst->stop();
        }
    };

    RGADummyPopulation population(16);
    StopListener listener;
    RGAInst inst(logger, RJSON::parse(R"({"ga": {}})"), &listener);
    listener.inst = &inst;
    inst.init(&population);
    inst.run(10);
    EXPECT_EQ(inst.gen(), 3u);

    // A new run resets the request
    inst.run(2);
    EXPECT_EQ(inst.gen(), 5u);
}


//------------------------------------------------------------------------------
TEST_F(RGABasicTests, minimize)
{
    RGADummyPopulation population(16);
    RGAInst inst(logger, RJSON::parse(R"({"ga": {"maximize": false, "mutation_frequency": 3, "inversion_frequency": 5}})"));
    EXPECT_FALSE(inst.mustMaximize());
    EXPECT_TRUE(inst.isBetter(1.0, 2.0));
    inst.init(&population);
    inst.run(20);

    // The best chromosome ever computed has the lowest fitness
    for (const auto& chromo : population.chromos())
        EXPECT_LE(population.best().fitness(), chromo.fitness());

    // The ranking goes from the lowest to the highest fitness
    const auto& ranked = inst.ranked();
    for (std::size_t i = 1; i < ranked.size(); ++i)
        EXPECT_LE(population[ranked[i - 1]].fitness(), population[ranked[i]].fitness());

    // Cannot be changed once initialized
    EXPECT_THROW(inst.updateConfig(R"({"ga": {"maximize": true}})"), RGAException);
    EXPECT_FALSE(inst.mustMaximize());
}


//------------------------------------------------------------------------------
TEST_F(RGABasicTests, promethee)
{
    // Same chromosomes, but the fitness is the Fi of PROMETHEE II:
    // maximize the value and minimize the number of bits set.
    RGADummyPopulation population(16);
    RGAInst inst(logger, RJSON::parse(R"({"ga": {"mutation_frequency": 3, "inversion_frequency": 5,
        "fitness": {"type": "promethee", "criteria": [
            {"name": "value", "type": "maximize", "p": 0.2, "q": 0.05, "weight": 2},
            {"name": "bits", "type": "minimize"}
        ]}}})"));
    EXPECT_EQ(inst.fitnessStrategy().name(), "promethee");
    inst.init(&population);
    inst.run(20);

    // The Fi are in [-1, 1] and the best chromosome ever computed (which is part of the last PROMETHEE session) is at
    // least as good as the others.
    for (const auto& chromo : population.chromos())
    {
        EXPECT_EQ(chromo.criteria().size(), 2u);
        EXPECT_GE(chromo.fitness(), -1.0);
        EXPECT_LE(chromo.fitness(), 1.0);
        EXPECT_GE(population.best().fitness(), chromo.fitness());
    }

    // The ranking is consistent with the Fi
    const auto& ranked = inst.ranked();
    for (std::size_t i = 1; i < ranked.size(); ++i)
        EXPECT_GE(population[ranked[i - 1]].fitness(), population[ranked[i]].fitness());

    // The fitness strategy cannot be changed once initialized
    EXPECT_THROW(inst.updateConfig(R"({"ga": {"fitness": {"type": "direct"}}})"), RGAException);
}


//------------------------------------------------------------------------------
TEST_F(RGABasicTests, fitnessConfig)
{
    // Direct by default
    RGAInst inst(logger, RJSON::parse(R"({"ga": {}})"));
    EXPECT_EQ(inst.fitnessStrategy().name(), "direct");

    // Invalid configurations
    EXPECT_THROW(RGAInst(logger, RJSON::parse(R"({"ga": {"fitness": {"type": "unknown"}}})")), RGAException);
    EXPECT_THROW(RGAInst(logger, RJSON::parse(R"({"ga": {"fitness": {"type": "promethee"}}})")), RGAException);
    EXPECT_THROW(RGAInst(logger, RJSON::parse(R"({"ga": {"fitness": {"type": "promethee", "criteria": []}}})")), RGAException);
    EXPECT_THROW(RGAInst(logger, RJSON::parse(R"({"ga": {"fitness": {"type": "promethee", "criteria": [{"name": "x"}]}}})")), RException);
    EXPECT_THROW(RGAInst(logger, RJSON::parse(R"({"ga": {"fitness": {"type": "promethee", "criteria": [{"type": "maximize", "p": 0.01, "q": 0.1}]}}})")), RException);

    // PROMETHEE requires the maximization
    EXPECT_THROW(RGAInst(logger, RJSON::parse(R"({"ga": {"maximize": false, "fitness": {"type": "promethee", "criteria": [{"type": "maximize"}]}}})")), RGAException);

    // The number of criteria of the chromosomes must match
    RGADummyPopulation population(8);
    RGAInst inst2(logger, RJSON::parse(R"({"ga": {"fitness": {"type": "promethee", "criteria": [{"type": "maximize"}]}}})"));
    inst2.init(&population);
    EXPECT_THROW(inst2.run(1), RGAException);
}
