/*

    EvoComb

    RGGABinPackingTests.cpp

    Copyright 2026 by Pascal Francq (pascal@francq.info).

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
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "r_google_test.hpp"
#include "ga/r_ga_inst.hpp"
#include "gga/r_gga_groups.hpp"
#include "r_bp_gga.hpp"
#include "tools/r_exception.hpp"
using namespace evocomb;
using namespace evocomb::bp;


//------------------------------------------------------------------------------
mLogModule("r.unit_tests");


//------------------------------------------------------------------------------
class RGGABinPackingTests : public RGoogleTest
{
protected:

    /**
     * Run a GGA on a generated problem.
     */
    std::unique_ptr<RPopulationBP> run(const RBPProblem& problem, const std::string& ga, const std::string& gga, const std::size_t nbGen) const
    {
        const auto config = RJSON::parse(R"({"ga": )" + ga + R"(, "gga": )" + gga + "}");
        auto population = std::make_unique<RPopulationBP>(24, RContextBP(problem), config);
        RGAInst inst(logger, config);
        inst.init(population.get());
        inst.run(nbGen);
        return population;
    }
};


//------------------------------------------------------------------------------
TEST_F(RGGABinPackingTests, groups)
{
    // Direct use of the groups (with bins of capacity 10)
    RBPProblem problem;
    std::ofstream(std::filesystem::temp_directory_path() / "evocomb_bp_test.txt") << "5\n4 6 3 7 5\n10\n3\n";
    problem.load(std::filesystem::temp_directory_path() / "evocomb_bp_test.txt");
    std::filesystem::remove(std::filesystem::temp_directory_path() / "evocomb_bp_test.txt");
    ASSERT_EQ(problem.objs().size(), 5u);
    EXPECT_EQ(problem.capacity(), 10u);
    EXPECT_EQ(problem.minBins(), 3u);
    EXPECT_EQ(problem.lowerBound(), 3u);

    RPopulationBP population(2, RContextBP(problem), RJSON::parse("{}"));
    RGAInst inst(logger, RJSON::parse(R"({"ga": {}})"));
    inst.init(&population);
    inst.run(0);        // Only the random construction
    const auto& best = population.best();
    EXPECT_EQ(best.nbAssigned(), 5u);
    EXPECT_NO_THROW(best.verifyGroups());

    // The best solution has a fitness in ]0, 1]
    EXPECT_GT(best.fitness(), 0.0);
    EXPECT_LE(best.fitness(), 1.0);

    // Copy and comparison of groupings
    RChromoBP copy = best;
    EXPECT_TRUE(copy.isSame(best));
    EXPECT_DOUBLE_EQ(copy.adjustedRandIndex(best), 1.0);
    const auto id = copy.used().front();
    copy.releaseGroup(id);
    copy.computeOrd();
    EXPECT_FALSE(copy.isSame(best));
    EXPECT_EQ(copy.nbNotAssigned(), best.group(id).nbObjs());
    EXPECT_NO_THROW(copy.verifyGroups());

    // Invalid operations
    const auto free = copy.reserveGroup();
    EXPECT_THROW(copy.reserveGroup(free), RGAException);
    EXPECT_THROW(copy.insertObj(free, *copy.group(copy.used().front()).objs().front()), RGAException);
}


//------------------------------------------------------------------------------
TEST_F(RGGABinPackingTests, solve)
{
    // 30 bins exactly filled
    RBPProblem problem;
    problem.generate(30, 150, 42);
    const auto population = run(problem,
        R"({"mutation_frequency": 3, "strong_mutation_frequency": 7, "inversion_frequency": 5, "handle_duplicates": true, "verify": true})",
        R"({"heuristic": "first_fit"})", 60);

    // The best solution is valid and optimal: the minimal number of bins, all exactly filled (so the fitness is 1).
    const auto& best = population->best();
    EXPECT_EQ(best.nbAssigned(), problem.objs().size());
    EXPECT_NO_THROW(best.verifyGroups());
    EXPECT_EQ(best.nbUsed(), problem.minBins());
    EXPECT_DOUBLE_EQ(best.fitness(), 1.0);
    for (const auto id : best.used())
        EXPECT_EQ(best.group(id).size(), problem.capacity());
    for (const auto& chromo : population->chromos())
    {
        EXPECT_NO_THROW(chromo.verifyGroups());
        EXPECT_GE(best.fitness(), chromo.fitness());
    }
}


//------------------------------------------------------------------------------
TEST_F(RGGABinPackingTests, options)
{
    RBPProblem problem;
    problem.generate(15, 100, 7);

    // Crossover keeping the modified groups, no local optimization, construction with the first-fit descending
    // heuristic
    const auto population = run(problem,
        R"({"mutation_frequency": 2, "inversion_frequency": 3, "verify": true})",
        R"({"heuristic": "first_fit_descending", "empty_modified_groups": false, "local_optimize": false})", 20);
    EXPECT_FALSE(population->context().emptyModifiedGroups());
    EXPECT_FALSE(population->context().localOptimize());
    EXPECT_EQ(population->best().nbAssigned(), problem.objs().size());
    EXPECT_NO_THROW(population->best().verifyGroups());

    // Invalid parameters
    EXPECT_THROW(RPopulationBP(4, RContextBP(problem), RJSON::parse(R"({"gga": {"local_optimize": 1}})")), RGAException);
    RPopulationBP pop2(4, RContextBP(problem), RJSON::parse(R"({"gga": {"heuristic": "unknown"}})"));
    RGAInst inst(logger, RJSON::parse(R"({"ga": {}})"));
    EXPECT_THROW(inst.init(&pop2), RGAException);
}


//------------------------------------------------------------------------------
TEST_F(RGGABinPackingTests, threads)
{
    // The result does not depend on the number of threads
    RBPProblem problem;
    problem.generate(20, 120, 3);
    const std::string ga = R"({"mutation_frequency": 3, "strong_mutation_frequency": 5, "inversion_frequency": 4, "handle_duplicates": true, "nb_threads": )";
    const auto pop1 = run(problem, ga + "1}", "{}", 25);
    const auto pop4 = run(problem, ga + "4}", "{}", 25);
    for (std::size_t i = 0; i < pop1->nbChromos(); ++i)
    {
        EXPECT_EQ((*pop1)[i].nbUsed(), (*pop4)[i].nbUsed());
        EXPECT_EQ((*pop1)[i].fitness(), (*pop4)[i].fitness());
        EXPECT_TRUE((*pop1)[i].isSame((*pop4)[i]));
    }
}


//------------------------------------------------------------------------------
TEST_F(RGGABinPackingTests, json)
{
    // Objects given by their sizes
    RBPProblem problem;
    problem.read(RJSON::parse(R"({"capacity": 10, "objects": [6, 4, 5, 5, 3], "min_bins": 3, "config": {}})"));
    ASSERT_EQ(problem.objs().size(), 5u);
    EXPECT_EQ(problem.capacity(), 10u);
    EXPECT_EQ(problem.minBins(), 3u);
    EXPECT_EQ(problem.objs()[2].size(), 5u);

    // Generated problem (the same as generate)
    RBPProblem generated;
    generated.read(RJSON::parse(R"({"generate": {"nb_bins": 5, "capacity": 100, "seed": 7}})"));
    RBPProblem reference;
    reference.generate(5, 100, 7);
    ASSERT_EQ(generated.objs().size(), reference.objs().size());
    for (std::size_t i = 0; i < reference.objs().size(); ++i)
        EXPECT_EQ(generated.objs()[i].size(), reference.objs()[i].size());

    // Invalid problems
    EXPECT_THROW(problem.read(RJSON::parse(R"({"objects": [1]})")), RException);
    EXPECT_THROW(problem.read(RJSON::parse(R"({"capacity": 10, "objects": []})")), RException);
    EXPECT_THROW(problem.read(RJSON::parse(R"({"capacity": 10, "objects": [11]})")), RException);
    EXPECT_THROW(problem.read(RJSON::parse(R"({"capacity": 10, "objects": [-1]})")), RException);
    EXPECT_THROW(problem.read(RJSON::parse(R"({"generate": {"nb_bins": 5}})")), RException);

    // Export of a solution: each object appears once, and the sizes of the bins are correct
    problem.read(RJSON::parse(R"({"capacity": 10, "objects": [6, 4, 5, 5, 3]})"));
    const auto population = run(problem, R"({"verify": true})", R"({"heuristic": "first_fit"})", 5);
    const auto json = RJSON::parse(population->best().toJSON()->dump());
    ASSERT_EQ(json.size(), population->best().nbUsed());
    std::vector<int> seen(problem.objs().size(), 0);
    for (std::size_t i = 0; i < json.size(); ++i)
    {
        long long total = 0;
        ASSERT_EQ(json[i]["objects"].size(), json[i]["sizes"].size());
        for (std::size_t j = 0; j < json[i]["objects"].size(); ++j)
        {
            const auto id = static_cast<std::size_t>(json[i]["objects"][j].toInteger());
            const auto size = json[i]["sizes"][j].toInteger();
            ++seen[id];
            EXPECT_EQ(size, static_cast<long long>(problem.objs()[id].size()));
            total += size;
        }
        EXPECT_EQ(json[i]["size"].toInteger(), total);
        EXPECT_LE(total, 10);
    }
    for (const auto nb : seen)
        EXPECT_EQ(nb, 1);
}
