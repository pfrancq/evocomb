/*

    EvoComb

    RSGGATests.cpp

    Tests of the Similarity-based Grouping Genetic Algorithm.

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
// include files for ANSI C/C++
#include <memory>
#include <random>
#include <string>
#include <vector>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "r_google_test.hpp"
#include "ga/r_ga_inst.hpp"
#include "sgga/r_sgga_chromo.hpp"
#include "tools/r_exception.hpp"
using namespace evocomb;


//------------------------------------------------------------------------------
mLogModule("r.unit_tests");


//------------------------------------------------------------------------------
/**
 * Object to group.
 */
class SGObj
{
public:
    explicit SGObj(const std::size_t id) : id_(id) {}
    [[nodiscard]] std::size_t id() const { return id_; }
private:
    std::size_t id_;
};


//------------------------------------------------------------------------------
class RSGGATests : public RGoogleTest
{
protected:

    static constexpr std::size_t NbClusters = 6;
    static constexpr std::size_t ClusterSize = 8;
    static constexpr std::size_t NbObjs = NbClusters * ClusterSize;

    using Solution = RSGGASolution<RSGGAGroup<SGObj>, SGObj>;
    using Operators = RSGGAOperators<RSGGAGroup<SGObj>, SGObj>;

    /**
     * Build the objects and synthetic data with known clusters: the objects of the same cluster are similar, some of
     * them agree, and some objects of different clusters disagree. Object i of the first cluster and object i of the
     * second cluster have the same parent.
     */
    RSGGATests() : data_(NbObjs)
    {
        for (std::size_t i = 0; i < NbObjs; ++i)
        {
            objs_.push_back(std::make_unique<SGObj>(i));
            ptrs_.push_back(objs_.back().get());
        }
        std::mt19937 random(4321);
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        for (std::size_t i = 0; i < NbObjs; ++i)
            for (std::size_t j = i + 1; j < NbObjs; ++j)
            {
                const auto same = cluster(i) == cluster(j);
                data_.sim(i, j, same ? 0.5 + 0.4 * dist(random) : 0.3 * dist(random));
                if (same && (dist(random) < 0.3))
                    data_.agreement(i, j, 0.8 + 0.2 * dist(random));
                if ((!same) && (dist(random) < 0.2))
                    data_.disagreement(i, j, 0.9);
            }
        for (std::size_t i = 0; i < NbObjs; i += 5)
            data_.social(i, true);
        for (std::size_t i = 0; i < ClusterSize; ++i)
        {
            data_.parent(i, i);
            data_.parent(i + ClusterSize, i);
        }
    }

    /**
     * Get the cluster of an object.
     */
    static std::size_t cluster(const std::size_t id) { return id / ClusterSize; }

    /**
     * Get the configuration.
     * @param nbThreads          Number of threads.
     * @param sgga               Content of the "sgga" object.
     */
    static RJSON config(const std::size_t nbThreads, const std::string& sgga = R"("min_agreement": 0.8, "min_disagreement": 0.8, "nb_max_objs": 12)")
    {
        return RJSON::parse(R"({
            "ga": {"nb_threads": )" + std::to_string(nbThreads) + R"(, "mutation_frequency": 5, "strong_mutation_frequency": 8,
                   "inversion_frequency": 10, "handle_duplicates": true, "verify": true,
                   "fitness": {"type": "promethee", "criteria": [
                        {"name": "similarity", "type": "maximize", "p": 0.2, "q": 0.05},
                        {"name": "agreement", "type": "maximize", "p": 0.2, "q": 0.05},
                        {"name": "disagreement", "type": "minimize", "p": 0.2, "q": 0.05}]}},
            "sgga": {)" + sgga + "}}");
    }

    /**
     * Run the SGGA.
     * @param config             Configuration.
     * @param nbGen              Number of generations.
     */
    std::unique_ptr<RSGGAPopulation<SGObj>> run(const RJSON& config, const std::size_t nbGen)
    {
        auto population = std::make_unique<RSGGAPopulation<SGObj>>(16, RSGGAContext<SGObj>(ptrs_, data_), config);
        RGAInst inst(logger, config);
        inst.init(population.get());
        inst.run(nbGen);
        return population;
    }

    /**
     * Build the known clustering in a solution.
     * @param groups             Solution.
     */
    void truth(RGGAGroups<RSGGAGroup<SGObj>, SGObj>& groups) const
    {
        groups.clearGroups();
        for (std::size_t c = 0; c < NbClusters; ++c)
        {
            const auto grp = groups.reserveGroup();
            for (std::size_t i = c * ClusterSize; i < (c + 1) * ClusterSize; ++i)
                groups.insertObj(grp, *ptrs_[i]);
        }
        groups.computeOrd();
    }

    /**
     * Verify that a solution respects the hard constraints: maximal size of the groups (12), no disagreement ratio
     * greater or equal to 0.8 and no common parent inside a group.
     * @param groups             Solution.
     */
    void verifyConstraints(const RGGAGroups<RSGGAGroup<SGObj>, SGObj>& groups) const
    {
        for (const auto id : groups.used())
        {
            const auto& objs = groups.group(id).objs();
            EXPECT_LE(objs.size(), 12u);
            for (const auto obj1 : objs)
                for (const auto obj2 : objs)
                {
                    if (obj1 == obj2)
                        continue;
                    EXPECT_LT(data_.disagreement(obj1->id(), obj2->id()), 0.8);
                    if (data_.parent(obj1->id()) != cNoRef)
                    {
                        EXPECT_NE(data_.parent(obj1->id()), data_.parent(obj2->id()));
                    }
                }
        }
    }

    //-----------------------------------------------------
    std::vector<std::unique_ptr<SGObj>> objs_;
    std::vector<const SGObj*> ptrs_;
    RSGGAMatrixData data_;
};


//------------------------------------------------------------------------------
TEST_F(RSGGATests, context)
{
    // Default values
    RSGGAContext<SGObj> context(ptrs_, data_);
    context.configure(RJSON::parse("{}"));
    EXPECT_EQ(context.heuristic(), "sg_first_fit");
    EXPECT_DOUBLE_EQ(context.minAgreement(), 1.0);
    EXPECT_EQ(context.nbMaxObjs(), 0u);
    EXPECT_EQ(context.maxKMeans(), 10u);
    EXPECT_EQ(context.nbDivChromos(), 2u);
    EXPECT_TRUE(context.criteria().isArray());
    EXPECT_TRUE(context.closeObjs(0).size() == 0);

    // Values read and close objects
    context.configure(config(1));
    EXPECT_DOUBLE_EQ(context.minDisagreement(), 0.8);
    EXPECT_EQ(context.nbMaxObjs(), 12u);
    for (std::size_t i = 0; i < NbObjs; ++i)
        for (const auto& [id, value] : context.closeObjs(i))
        {
            EXPECT_GE(value, 0.8);
            EXPECT_EQ(cluster(id), cluster(i));
        }

    // Compatibility
    EXPECT_FALSE(context.compatible(*ptrs_[0], *ptrs_[ClusterSize]));      // Same parent
    EXPECT_TRUE(context.compatible(*ptrs_[0], *ptrs_[1]));

    // Invalid values
    EXPECT_THROW(context.configure(RJSON::parse(R"({"sgga": {"convergence": "low"}})")), RGAException);
    EXPECT_THROW(context.configure(RJSON::parse(R"({"sgga": {"max_kmeans": -1}})")), RGAException);
    EXPECT_THROW(context.configure(RJSON::parse(R"({"sgga": {"criteria": 3}})")), RGAException);
    RSGGAMatrixData small(3);
    RSGGAContext<SGObj> wrong(ptrs_, small);
    EXPECT_THROW(wrong.configure(RJSON::parse("{}")), RGAException);
}


//------------------------------------------------------------------------------
TEST_F(RSGGATests, group)
{
    RSGGAContext<SGObj> context(ptrs_, data_);
    context.configure(config(1));
    Solution sol;
    sol.init(context);
    const auto grp = sol.reserveGroup();
    for (std::size_t i = 0; i < 3; ++i)
        sol.insertObj(grp, *ptrs_[i]);
    const auto& group = sol.group(grp);

    // Centroid: the object with the highest sum of similarities
    double best = -1.0;
    std::size_t centroid = cNoRef;
    for (std::size_t i = 0; i < 3; ++i)
    {
        double sum = 0.0;
        for (std::size_t j = 0; j < 3; ++j)
            if (i != j)
                sum += data_.sim(i, j);
        if (sum > best)
        {
            best = sum;
            centroid = i;
        }
    }
    EXPECT_EQ(group.centroid()->id(), centroid);
    EXPECT_NEAR(group.avgIntraSim(), best / 2.0, 1e-12);

    // The cache is updated after a modification
    sol.deleteObj(*ptrs_[centroid]);
    EXPECT_NE(group.centroid()->id(), centroid);

    // Constraints
    const auto grp2 = sol.reserveGroup();
    sol.insertObj(grp2, *ptrs_[3]);
    EXPECT_FALSE(sol.group(grp2).canInsert(*ptrs_[ClusterSize + 3]));     // Same parent
    double maxSim;
    EXPECT_TRUE(group.canInsert(*ptrs_[5], maxSim));
    EXPECT_GE(maxSim, 0.5);
}


//------------------------------------------------------------------------------
TEST_F(RSGGATests, operators)
{
    RSGGAContext<SGObj> context(ptrs_, data_);
    context.configure(config(1));
    RSGGAWork<SGObj> work;
    RGARandom random(1);
    Solution sol;
    sol.init(context);
    truth(sol);
    std::vector<double> criteria;
    Operators::evaluate(sol, criteria);
    ASSERT_EQ(criteria.size(), 3u);
    EXPECT_GT(criteria[0], 1.0);
    EXPECT_DOUBLE_EQ(criteria[2], 0.0);

    // A k-Means keeps the known clustering
    Solution copy;
    copy.init(context);
    copy.assign(sol);
    Operators::kMeans(context, copy, work, random);
    copy.computeOrd();
    EXPECT_NEAR(copy.adjustedRandIndex(sol), 1.0, 1e-12);

    // Division and merge
    copy.assign(sol);
    EXPECT_TRUE(Operators::divideWorstObjects(context, copy, work));
    EXPECT_EQ(copy.nbUsed(), NbClusters + 1);
    EXPECT_NO_THROW(copy.verifyGroups());
    copy.assign(sol);
    Operators::mergeBestGroups(context, copy, work);
    EXPECT_LE(copy.nbUsed(), NbClusters);
    EXPECT_EQ(copy.nbAssigned(), NbObjs);
    EXPECT_NO_THROW(copy.verifyGroups());
    verifyConstraints(copy);

    // A single social object is moved
    copy.assign(sol);
    copy.deleteObj(*ptrs_[6]);
    copy.insertObj(copy.reserveGroup(), *ptrs_[6]);
    EXPECT_EQ(Operators::emptyGroups(context, copy, work, false), 0u);      // Object 6 is not social
    copy.deleteObj(*ptrs_[10]);
    copy.insertObj(copy.reserveGroup(), *ptrs_[10]);
    EXPECT_EQ(Operators::emptyGroups(context, copy, work, false), 1u);      // Object 10 is social
    EXPECT_EQ(copy.nbUsed(), NbClusters + 1);
}


//------------------------------------------------------------------------------
TEST_F(RSGGATests, clustering)
{
    const auto population = run(config(1), 30);
    const auto& best = population->best();

    // Valid solution
    EXPECT_EQ(best.nbAssigned(), NbObjs);
    EXPECT_NO_THROW(best.verifyGroups());
    EXPECT_EQ(best.criteria().size(), 3u);
    EXPECT_GT(best.simJ(), 1.0);

    // Social objects are not alone
    for (std::size_t i = 0; i < NbObjs; ++i)
    {
        if (data_.isSocial(i))
        {
            EXPECT_GT(best.groupOf(*ptrs_[i])->nbObjs(), 1u);
        }
    }

    // Close to the known clustering
    Solution known;
    known.init(population->context());
    truth(known);
    const auto ari = best.adjustedRandIndex(known);
    mLogInfo(logger, "SGGA: ", best.nbUsed(), " groups, adjusted Rand index = ", ari);
    EXPECT_GT(ari, 0.8);
}


//------------------------------------------------------------------------------
TEST_F(RSGGATests, constraints)
{
    // Every chromosome respects the hard constraints
    const auto population = run(config(1), 10);
    for (std::size_t c = 0; c < population->nbChromos(); ++c)
        verifyConstraints((*population)[c]);
}


//------------------------------------------------------------------------------
TEST_F(RSGGATests, noOptimization)
{
    // Without optimizations, the solutions are still valid
    const auto population = run(config(1, R"("min_disagreement": 0.8, "nb_div_chromos": 0)"), 10);
    for (std::size_t c = 0; c < population->nbChromos(); ++c)
    {
        EXPECT_EQ((*population)[c].nbAssigned(), NbObjs);
        EXPECT_NO_THROW((*population)[c].verifyGroups());
    }
}


//------------------------------------------------------------------------------
TEST_F(RSGGATests, threads)
{
    // The result does not depend on the number of threads
    const auto pop1 = run(config(1), 10);
    const auto pop4 = run(config(4), 10);
    for (std::size_t i = 0; i < pop1->nbChromos(); ++i)
    {
        EXPECT_TRUE((*pop1)[i].isSame((*pop4)[i]));
        EXPECT_EQ((*pop1)[i].fitness(), (*pop4)[i].fitness());
    }
}
