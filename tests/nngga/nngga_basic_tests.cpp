/*

    EvoComb

    RNNGGATests.cpp

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
#include <memory>
#include <random>
#include <string>
#include <vector>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "r_google_test.hpp"
#include "ga/r_ga_inst.hpp"
#include "nngga/r_nngga_chromo.hpp"
using namespace evocomb;


//------------------------------------------------------------------------------
mLogModule("r.unit_tests");


//------------------------------------------------------------------------------
/**
 * Object to group.
 */
class NNObj
{
public:
    explicit NNObj(const std::size_t id) : id_(id) {}
    [[nodiscard]] std::size_t id() const { return id_; }
private:
    std::size_t id_;
};


//------------------------------------------------------------------------------
class RNNGGATests : public RGoogleTest
{
protected:

    static constexpr std::size_t NbClusters = 6;
    static constexpr std::size_t ClusterSize = 8;
    static constexpr std::size_t NbObjs = NbClusters * ClusterSize;

    /**
     * Build the objects and synthetic data with known clusters: the objects of the same cluster are similar, some of
     * them agree, and some objects of different clusters disagree.
     */
    RNNGGATests() : data_(NbObjs, 10)
    {
        for (std::size_t i = 0; i < NbObjs; ++i)
        {
            objs_.push_back(std::make_unique<NNObj>(i));
            ptrs_.push_back(objs_.back().get());
        }
        std::mt19937 random(1234);
        std::uniform_real_distribution dist(0.0, 1.0);
        for (std::size_t i = 0; i < NbObjs; ++i)
            for (std::size_t j = i + 1; j < NbObjs; ++j)
            {
                const auto same = cluster(i) == cluster(j);
                const auto sim = same ? 0.5 + 0.4 * dist(random) : 0.3 * dist(random);
                data_.simMatrix().add(i, j, sim);
                data_.simMatrix().add(j, i, sim);
                if (same && dist(random) < 0.3)
                {
                    const auto agree = 0.8 + 0.2 * dist(random);
                    data_.agreementMatrix().add(i, j, agree);
                    data_.agreementMatrix().add(j, i, agree);
                }
                if (!same && dist(random) < 0.2)
                {
                    data_.disagreementMatrix().add(i, j, 0.9);
                    data_.disagreementMatrix().add(j, i, 0.9);
                }
            }
        for (std::size_t i = 0; i < NbObjs; i += 5)
            data_.social(i, true);
    }

    /**
     * Get the cluster of an object.
     */
    static std::size_t cluster(const std::size_t id) { return id / ClusterSize; }

    /**
     * Run the NNGGA.
     */
    std::unique_ptr<RNNGGAPopulation<NNObj>> run(const std::size_t nbThreads, const std::size_t nbGen)
    {
        const auto config = RJSON::parse(R"({
            "ga": {"nb_threads": )" + std::to_string(nbThreads) + R"(, "mutation_frequency": 5, "strong_mutation_frequency": 8,
                   "inversion_frequency": 10, "handle_duplicates": true, "verify": true,
                   "fitness": {"type": "promethee", "criteria": [
                        {"name": "similarity", "type": "maximize", "p": 0.2, "q": 0.05},
                        {"name": "agreement", "type": "maximize", "p": 0.2, "q": 0.05},
                        {"name": "disagreement", "type": "minimize", "p": 0.2, "q": 0.05}]}},
            "nngga": {"min_agreement": 0.8, "min_disagreement": 0.8, "nb_max_objs": 12}})");
        auto population = std::make_unique<RNNGGAPopulation<NNObj>>(16, RNNGGAContext(ptrs_, data_), config);
        RGAInst inst(logger, config);
        inst.init(population.get());
        inst.run(nbGen);
        return population;
    }

    /**
     * Build the known clustering (with the structure of a chromosome).
     */
    [[nodiscard]] RNNGGAChromo<NNObj> truth(const RNNGGAChromo<NNObj>& model) const
    {
        auto res = model;
        res.clearGroups();
        for (std::size_t c = 0; c < NbClusters; ++c)
        {
            const auto grp = res.reserveGroup();
            for (std::size_t i = c * ClusterSize; i < (c + 1) * ClusterSize; ++i)
                res.insertObj(grp, *ptrs_[i]);
        }
        res.computeOrd();
        return res;
    }

    //-----------------------------------------------------
    std::vector<std::unique_ptr<NNObj>> objs_;
    std::vector<const NNObj*> ptrs_;
    RNNGGAMatrixData data_;
};


//------------------------------------------------------------------------------
TEST_F(RNNGGATests, data)
{
    // The 10 nearest neighbors are kept, the most similar first
    const auto& sims = data_.sims(0);
    EXPECT_EQ(sims.size(), 10u);
    for (std::size_t i = 1; i < sims.size(); ++i)
        EXPECT_GE(sims.value(i - 1), sims.value(i));
    for (std::size_t i = 0; i < ClusterSize - 1; ++i)
        EXPECT_EQ(cluster(sims[i].id_), cluster(0));
    EXPECT_TRUE(data_.isSocial(0));
    EXPECT_FALSE(data_.isSocial(1));
}


//------------------------------------------------------------------------------
TEST_F(RNNGGATests, clustering)
{
    const auto population = run(1, 30);
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
    const auto ari = best.adjustedRandIndex(truth(best));
    mLogInfo(logger, "NNGGA: ", best.nbUsed(), " groups, adjusted Rand index = ", ari);
    EXPECT_GT(ari, 0.8);
}


//------------------------------------------------------------------------------
TEST_F(RNNGGATests, constraints)
{
    // Every chromosome respects the maximal group size (12) and never puts two objects with a high disagreement ratio
    // (>= 0.8) in the same group
    const auto population = run(1, 10);
    for (std::size_t c = 0; c < population->nbChromos(); ++c)
    {
        const auto& chromo = (*population)[c];
        for (const auto id : chromo.used())
        {
            const auto& objs = chromo.group(id).objs();
            EXPECT_LE(objs.size(), 12u);
            for (const auto obj : objs)
                for (const auto other : objs)
                    EXPECT_LT(data_.disagreements(obj->id()).valueOf(other->id()), 0.8);
        }
    }
}


//------------------------------------------------------------------------------
TEST_F(RNNGGATests, threads)
{
    // The result does not depend on the number of threads
    const auto pop1 = run(1, 10);
    const auto pop4 = run(4, 10);
    for (std::size_t i = 0; i < pop1->nbChromos(); ++i)
    {
        EXPECT_TRUE((*pop1)[i].isSame((*pop4)[i]));
        EXPECT_EQ((*pop1)[i].fitness(), (*pop4)[i].fitness());
    }
}


//------------------------------------------------------------------------------
TEST_F(RNNGGATests, config)
{
    // Default values
    RNNGGAContext context(ptrs_, data_);
    context.configure(RJSON::parse("{}"));
    EXPECT_EQ(context.heuristic(), "nn_first_fit");
    EXPECT_DOUBLE_EQ(context.minAgreement(), 1.0);
    EXPECT_EQ(context.nbMaxObjs(), 0u);

    // Values read
    context.configure(RJSON::parse(R"({"nngga": {"min_sim_level": 0.1, "nb_min_objs": 2}})"));
    EXPECT_DOUBLE_EQ(context.minSimLevel(), 0.1);
    EXPECT_EQ(context.nbMinObjs(), 2u);

    // Invalid values
    EXPECT_THROW(context.configure(RJSON::parse(R"({"nngga": {"min_agreement": "high"}})")), RGAException);
    EXPECT_THROW(context.configure(RJSON::parse(R"({"nngga": {"nb_max_objs": -1}})")), RGAException);
    RNNGGAMatrixData small(3, 2);
    RNNGGAContext<NNObj> wrong(ptrs_, small);
    EXPECT_THROW(wrong.configure(RJSON::parse("{}")), RGAException);
}
