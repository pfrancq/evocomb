/*

    EvoComb

    RHGATests.cpp

    Tests of the Hierarchical Genetic Algorithm.

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
#include "hga/r_hga_chromo.hpp"
#include "r_hierarchy_hga.hpp"
#include "r_hierarchy_problem.hpp"
#include "tools/r_exception.hpp"
using namespace evocomb;
using namespace evocomb::hierarchy;


//------------------------------------------------------------------------------
mLogModule("r.unit_tests");


//------------------------------------------------------------------------------
class RHGATests : public RGoogleTest
{
protected:

    using Tree = RHGATree<RHGANode<RHGAObj>, RHGAObj>;

    /**
     * Create some objects.
     * @param attrs              Attributes of each object.
     */
    void create(const std::vector<RHGAAttrList>& attrs)
    {
        objs_.clear();
        ptrs_.clear();
        for (std::size_t i = 0; i < attrs.size(); ++i)
        {
            objs_.push_back(std::make_unique<RHGAObj>(i, "o" + std::to_string(i), attrs[i]));
            ptrs_.push_back(objs_.back().get());
        }
    }

    /**
     * Build a taxonomy: 3 categories, each one with 3 sub-categories, each one with 4 objects (two of them having the
     * same attributes). Attribute 100+c is the category c, 200+s the sub-category s, and 300+o is specific to object o.
     */
    std::string taxonomy() const
    {
        std::string json = R"({"objects": [)";
        std::size_t o = 0;
        for (std::size_t c = 0; c < 3; ++c)
            for (std::size_t s = 0; s < 3; ++s)
                for (std::size_t i = 0; i < 4; ++i, ++o)
                {
                    if (o > 0)
                        json += ',';
                    json += R"({"name": "o)" + std::to_string(o) + R"(", "attributes": ["c)" + std::to_string(c) +
                            R"(", "s)" + std::to_string(3 * c + s) + R"(")";
                    if (i < 3)
                        json += R"(, "x)" + std::to_string(o) + R"(")";
                    json += "]}";
                }
        return json + "]}";
    }

    /**
     * Run the HGA on the taxonomy.
     * @param nbThreads          Number of threads.
     * @param nbGen              Number of generations.
     */
    std::unique_ptr<RPopulationHierarchy> run(const std::size_t nbThreads, const std::size_t nbGen)
    {
        problem_.read(RJSON::parse(taxonomy()));
        const auto config = RJSON::parse(R"({"ga": {"maximize": false, "nb_threads": )" + std::to_string(nbThreads) +
                                         R"(, "mutation_frequency": 5, "handle_duplicates": true, "verify": true}})");
        auto population = std::make_unique<RPopulationHierarchy>(16, RHGAContext<RHGAObj>(problem_.objs()), config);
        RGAInst inst(logger, config);
        inst.init(population.get());
        inst.run(nbGen);
        return population;
    }

    //-----------------------------------------------------
    std::vector<std::unique_ptr<RHGAObj>> objs_;
    std::vector<const RHGAObj*> ptrs_;
    RHierarchyProblem problem_;
};


//------------------------------------------------------------------------------
TEST_F(RHGATests, attrList)
{
    const RHGAAttrList a{3, 1, 2, 3};
    const RHGAAttrList b{1, 2};
    const RHGAAttrList c{2, 5};
    EXPECT_EQ(a.size(), 3u);
    EXPECT_EQ(a.toString(), "1,2,3");
    EXPECT_TRUE(b.isParentOf(a));
    EXPECT_FALSE(a.isParentOf(b));
    EXPECT_TRUE(RHGAAttrList().isParentOf(a));
    EXPECT_TRUE(a.intersects(c));
    EXPECT_FALSE(b.intersects(RHGAAttrList{5, 6}));
    EXPECT_EQ(a.nbCommon(c), 1u);
    EXPECT_EQ(RHGAAttrList::inter(a, c), RHGAAttrList{2});
    EXPECT_EQ(RHGAAttrList::diff(a, b), RHGAAttrList{3});
    RHGAAttrList d;
    d.insert(4);
    d.insert(1);
    d.insert(4);
    EXPECT_EQ(d, (RHGAAttrList{1, 4}));
    EXPECT_TRUE(d.contains(4));
}


//------------------------------------------------------------------------------
TEST_F(RHGATests, tree)
{
    create({{1}, {1, 2}, {1, 2, 3}, {4}});
    Tree tree;
    tree.initTree(ptrs_);
    const auto n1 = tree.addNode(cNoRef, {1});
    const auto n12 = tree.addNode(n1, {1, 2});
    const auto n123 = tree.addNode(n12, {1, 2, 3});
    const auto n4 = tree.addNode(cNoRef, {4});
    tree.insertObj(n1, *ptrs_[0]);
    tree.insertObj(n12, *ptrs_[1]);
    tree.insertObj(n123, *ptrs_[2]);
    tree.insertObj(n4, *ptrs_[3]);
    EXPECT_NO_THROW(tree.verifyTree(true));
    EXPECT_EQ(tree.nbNodes(), 4u);
    EXPECT_EQ(tree.depth(), 3u);
    std::vector<const RHGAObj*> objs;
    tree.allObjs(n12, objs);
    EXPECT_EQ(objs.size(), 2u);

    // Invalid operations
    EXPECT_THROW(tree.moveNode(n123, n1), RGAException);
    EXPECT_THROW(tree.insertObj(n4, *ptrs_[0]), RGAException);

    // Copy without the branch of n12 and without object 3
    std::vector<char> skip{0, 0, 0, 1};
    Tree copy;
    copy.initTree(ptrs_);
    const auto parent = copy.copyExceptBranch(tree, n12, &skip);
    EXPECT_EQ(copy.nbNodes(), 1u);      // n4 is removed because empty
    EXPECT_EQ(copy.node(parent).attrs(), RHGAAttrList{1});
    EXPECT_EQ(copy.nbAssigned(), 1u);
    copy.copyBranch(parent, tree, n12);
    EXPECT_EQ(copy.nbNodes(), 3u);
    EXPECT_THROW(copy.verifyTree(true), RGAException);     // Object 3 is not attached
    EXPECT_NO_THROW(copy.verifyTree(false));

    // Canonical representation
    copy.insertObj(copy.addNode(cNoRef, {4}), *ptrs_[3]);
    tree.computeCanonical();
    copy.computeCanonical();
    EXPECT_TRUE(copy.sameTree(tree));

    // Deletion of a branch and clean up
    tree.deleteObj(*ptrs_[0]);
    tree.deleteNode(n12);
    EXPECT_EQ(tree.nbAssigned(), 1u);
    EXPECT_EQ(tree.cleanUp(), 1u);      // n1 is empty
    EXPECT_EQ(tree.nbNodes(), 1u);
    tree.computeCanonical();
    EXPECT_FALSE(copy.sameTree(tree));
}


//------------------------------------------------------------------------------
TEST_F(RHGATests, firstNode)
{
    create({{1, 2}, {1, 3}, {1, 2, 3, 4}, {1, 2, 3}, {5}});
    const RHGAContext<RHGAObj> context(ptrs_);
    RHGAFirstNodeHeuristic<RHGANode<RHGAObj>, RHGAObj> heuristic(context);
    RGARandom random(1);
    Tree tree;
    tree.initTree(ptrs_);
    // Attach all the objects and verify the structure
    heuristic.run(logger, tree, random);
    EXPECT_NO_THROW(tree.verifyTree(true));

    // {1, 2} and {1, 3} are children of an intermediate node {1}
    const auto node12 = tree.nodeId(0);
    const auto node13 = tree.nodeId(1);
    ASSERT_NE(tree.node(node12).parent(), cNoRef);
    EXPECT_EQ(tree.node(node12).parent(), tree.node(node13).parent());
    EXPECT_EQ(tree.node(tree.node(node12).parent()).attrs(), RHGAAttrList{1});

    // {5} is a top node
    EXPECT_EQ(tree.node(tree.nodeId(4)).parent(), cNoRef);
}


//------------------------------------------------------------------------------
TEST_F(RHGATests, subsetObject)
{
    // An object whose attributes are included in those of an existing node is attached to the intermediate node (and
    // not to a new node with the same attributes)
    create({{1, 2, 3}, {1, 2}});
    const RHGAContext<RHGAObj> context(ptrs_);
    RHGAFirstNodeHeuristic<RHGANode<RHGAObj>, RHGAObj> heuristic(context);
    RGARandom random(1);
    Tree tree;
    tree.initTree(ptrs_);
    tree.insertObj(tree.addNode(cNoRef, {1, 2, 3}), *ptrs_[0]);
    heuristic.run(logger, tree, random);
    EXPECT_NO_THROW(tree.verifyTree(true));
    EXPECT_EQ(tree.nbNodes(), 2u);
    EXPECT_EQ(tree.node(tree.nodeId(0)).parent(), tree.nodeId(1));
}


//------------------------------------------------------------------------------
TEST_F(RHGATests, problem)
{
    RHierarchyProblem problem;
    problem.read(RJSON::parse(R"({"objects": [{"name": "A", "attributes": ["x", "y"]}, {"attributes": ["y", "z"]}]})"));
    ASSERT_EQ(problem.objs().size(), 2u);
    EXPECT_EQ(problem.nbAttrs(), 3u);
    EXPECT_EQ(problem.objs()[0]->name(), "A");
    EXPECT_EQ(problem.objs()[1]->name(), "Object 1");
    EXPECT_EQ(problem.attrLabel(problem.objs()[1]->attrs()[1]), "z");

    // Invalid problems
    EXPECT_THROW(problem.read(RJSON::parse(R"({"objs": []})")), RException);
    EXPECT_THROW(problem.read(RJSON::parse(R"({"objects": [{"name": "A", "attributes": []}]})")), RException);
    EXPECT_THROW(problem.read(RJSON::parse(R"({"objects": [{"attributes": [1]}]})")), RException);
}


//------------------------------------------------------------------------------
TEST_F(RHGATests, hierarchy)
{
    const auto population = run(1, 20);
    const auto& best = population->best();
    EXPECT_NO_THROW(best.verifyTree(true));

    // The taxonomy is found: 3 categories with 3 sub-categories each, the specific objects being attached below their
    // sub-category. To reach an object: 3 choices among the top nodes, 3 in a category node and 3 + 1 in a sub-category
    // node, plus 1 in a specific node for the 27 specific objects: (9 * 10 + 27 * 11) / 36 = 10.75 choices.
    mLogInfo(logger, "HGA: ", best.nbNodes(), " nodes, depth ", best.depth(), ", ", best.avgChoices(), " choices");
    EXPECT_EQ(best.topNodes().size(), 3u);
    EXPECT_EQ(best.depth(), 3u);
    EXPECT_EQ(best.nbNodes(), 3u + 9u + 27u);
    EXPECT_DOUBLE_EQ(best.avgChoices(), 10.75);
    EXPECT_DOUBLE_EQ(best.fitness(), best.avgChoices());

    // Export: all the objects appear once
    const auto json = best.toJSON(problem_);
    const auto dump = json->dump();
    for (std::size_t o = 0; o < best.nbObjs(); ++o)
        EXPECT_NE(dump.find("\"o" + std::to_string(o) + "\""), std::string::npos);
    const auto parsed = RJSON::parse(dump);
    EXPECT_EQ(parsed.size(), 3u);
    EXPECT_EQ(parsed[0]["children"].size(), 3u);
}


//------------------------------------------------------------------------------
TEST_F(RHGATests, overlapping)
{
    // Objects with random (overlapping) attributes: the trees depend on the order of the objects, and the HGA improves
    // the initial ones
    std::mt19937 random(7);
    std::uniform_int_distribution<std::size_t> nb(2, 5);
    std::uniform_int_distribution<std::size_t> attr(0, 11);
    std::string json = R"({"objects": [)";
    for (std::size_t o = 0; o < 60; ++o)
    {
        if (o > 0)
            json += ',';
        json += R"({"attributes": [)";
        const auto n = nb(random);
        for (std::size_t i = 0; i < n; ++i)
            json += (i ? ",\"a" : "\"a") + std::to_string(attr(random)) + "\"";
        json += "]}";
    }
    problem_.read(RJSON::parse(json + "]}"));
    const auto config = RJSON::parse(R"({"ga": {"maximize": false, "mutation_frequency": 3, "verify": true}})");
    RPopulationHierarchy population(16, RHGAContext<RHGAObj>(problem_.objs()), config);
    RGAInst inst(logger, config);
    inst.init(&population);
    inst.run(1);
    const auto initial = population.best().fitness();
    inst.run(40);
    const auto& best = population.best();
    mLogInfo(logger, "HGA: ", initial, " -> ", best.fitness(), " choices (", best.nbNodes(), " nodes)");
    EXPECT_NO_THROW(best.verifyTree(true));
    EXPECT_LT(best.fitness(), initial);
    for (std::size_t i = 0; i < population.nbChromos(); ++i)
        EXPECT_NO_THROW(population[i].verifyTree(true));
}


//------------------------------------------------------------------------------
TEST_F(RHGATests, threads)
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
