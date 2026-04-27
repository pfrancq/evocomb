/*

    EvoComb

    RGA2DTests.cpp

    Tests of the 2D Placement Genetic Algorithm.

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
#include "ga2d/r_ga2d_population.hpp"
#include "r_vlsi_problem.hpp"
#include "tools/r_exception.hpp"
using namespace evocomb;
using namespace evocomb::vlsi;


//------------------------------------------------------------------------------
mLogModule("r.unit_tests");


//------------------------------------------------------------------------------
class RGA2DTests : public RGoogleTest
{
protected:

    /**
     * Build a random VLSI problem: 12 instances of 3 masters (including an L-shaped one), connected by random nets, and
     * a terminal of the circuit.
     * @param seed               Seed.
     */
    static std::string generate(const unsigned seed)
    {
        std::mt19937 random(seed);
        std::string json = R"({"size": [120, 120],
            "terminals": [{"id": "IN", "pins": [{"rect": [-2, 60, 0, 62]}]}],
            "masters": [
                {"id": "S", "polygon": [[0, 0], [10, 0], [10, 10], [0, 10]],
                 "terminals": [{"id": "A", "pins": [{"rect": [1, 1, 2, 2]}]}, {"id": "B", "pins": [{"rect": [8, 8, 9, 9]}]}]},
                {"id": "R", "polygon": [[0, 0], [30, 0], [30, 10], [0, 10]],
                 "terminals": [{"id": "A", "pins": [{"rect": [2, 4, 4, 6]}]}, {"id": "B", "pins": [{"rect": [26, 4, 28, 6]}]}]},
                {"id": "L", "polygon": [[0, 0], [20, 0], [20, 10], [10, 10], [10, 20], [0, 20]],
                 "terminals": [{"id": "A", "pins": [{"rect": [1, 18, 2, 19]}]}, {"id": "B", "pins": [{"rect": [18, 1, 19, 2]}]}]}
            ],
            "instances": [)";
        const char* masters[] = {"S", "R", "L"};
        for (int i = 0; i < 12; ++i)
            json += std::string(i ? "," : "") + R"({"id": "I)" + std::to_string(i) + R"(", "master": ")" + masters[i % 3] + "\"}";
        json += R"(], "nets": [)";
        std::uniform_int_distribution<int> inst(0, 11);
        std::uniform_int_distribution<int> term(0, 1);
        for (int n = 0; n < 10; ++n)
        {
            json += std::string(n ? "," : "") + R"({"id": "n)" + std::to_string(n) + R"(", "weight": )" + std::to_string(1 + n % 3) + R"(, "connect": [)";
            for (int k = 0; k < 3; ++k)
                json += std::string(k ? "," : "") + R"({"instance": "I)" + std::to_string(inst(random)) + R"(", "terminal": ")" + (term(random) ? "A" : "B") + "\"}";
            if (n == 0)
                json += R"(, {"terminal": "IN"})";
            json += "]}";
        }
        return json + "]}";
    }

    /**
     * Run the GA.
     * @param problem            Problem.
     * @param ga2d               Content of the "ga2d" object.
     * @param nbThreads          Number of threads.
     * @param nbGen              Number of generations.
     */
    std::unique_ptr<RGA2DPopulation<>> run(const RVLSIProblem& problem, const std::string& ga2d, const std::size_t nbThreads, const std::size_t nbGen)
    {
        const auto config = RJSON::parse(R"({"ga": {"maximize": false, "verify": true, "handle_duplicates": true, "mutation_frequency": 4,
            "nb_threads": )" + std::to_string(nbThreads) + R"(}, "ga2d": {)" + ga2d + "}}");
        auto population = std::make_unique<RGA2DPopulation<>>(12, RGA2DContext(problem.problem()), config);
        RGAInst inst(logger, config);
        inst.init(population.get());
        inst.run(nbGen);
        return population;
    }
};


//------------------------------------------------------------------------------
TEST_F(RGA2DTests, configs)
{
    RGA2DProblem problem;
    problem.limits(100, 100);

    // A square with a centered pin: a single configuration
    auto& square = problem.addObj("square", RPolygon(RRect(5, 5, 15, 15)));
    square.addConnector("c");
    square.addPin(0, {"p", "", RRect(9, 9, 11, 11)});

    // A rectangle without pins: two configurations
    problem.addObj("rect", RPolygon(RRect(0, 0, 20, 10)));

    // A rectangle with a pin near a corner: eight configurations
    auto& pinned = problem.addObj("pinned", RPolygon(RRect(0, 0, 20, 10)));
    pinned.addConnector("c");
    pinned.addPin(0, {"p", "", RRect(1, 1, 2, 2)});
    problem.init();

    ASSERT_EQ(problem.obj(0).configs().size(), 1u);
    EXPECT_EQ(problem.obj(0).configs()[0].centers[0][0], RPoint(5, 5));    // Translated with the shape
    EXPECT_EQ(problem.obj(1).configs().size(), 2u);
    EXPECT_EQ(problem.obj(1).configs()[1].width, 10);
    EXPECT_EQ(problem.obj(2).configs().size(), 8u);
    for (const auto& cfg : problem.obj(2).configs())
    {
        EXPECT_EQ(cfg.area, 200);
        EXPECT_TRUE(RRect(0, 0, cfg.width, cfg.height).contains(cfg.pins[0][0]));
    }

    // An object larger than the limits
    RGA2DProblem big;
    big.limits(10, 10);
    big.addObj("big", RPolygon(RRect(0, 0, 20, 5)));
    EXPECT_THROW(big.init(), RException);
}


//------------------------------------------------------------------------------
TEST_F(RGA2DTests, layout)
{
    RGA2DProblem problem;
    problem.limits(50, 50);
    for (int i = 0; i < 3; ++i)
    {
        auto& obj = problem.addObj("o" + std::to_string(i), RPolygon(RRect(0, 0, 10, 10)));
        obj.addConnector("c");
        obj.addPin(0, {"p", "", RRect(0, 0, 1, 1)});
    }
    problem.addBoardConnector("in").addPin({"p", "", RRect(-1, 0, 0, 1)});
    problem.addNet({"n", 2.0, {{0, 0}, {1, 0}, {2, 0}, {cNoRef, 0}}});
    problem.init();

    RGA2DLayout layout;
    layout.initLayout(problem);
    layout.place(0, 0, {0, 0});
    layout.place(1, 0, {20, 0});
    EXPECT_EQ(layout.nbPlaced(), 2u);
    EXPECT_EQ(layout.boundary(), RRect(0, 0, 30, 10));

    // Minimum spanning tree: board (-1,0) - o0 (0,0) - o1 (20,0)
    EXPECT_DOUBLE_EQ(layout.netLength(0), 1.0 + 20.0);
    layout.place(2, 0, {10, 30});
    EXPECT_DOUBLE_EQ(layout.netLength(0), 1.0 + 20.0 + 40.0);
    EXPECT_DOUBLE_EQ(layout.totalLength(true), 2.0 * 61.0);
    EXPECT_DOUBLE_EQ(layout.connectedWeight(0), 2.0);
    EXPECT_NO_THROW(layout.verifyLayout(true));

    // Overlap and limits
    layout.place(2, 0, {5, 5});
    EXPECT_THROW(layout.verifyLayout(true), RGAException);
    layout.place(2, 0, {45, 0});
    EXPECT_THROW(layout.verifyLayout(true), RGAException);
    layout.unplace(2);
    EXPECT_NO_THROW(layout.verifyLayout(false));
    EXPECT_THROW(layout.verifyLayout(true), RGAException);
}


//------------------------------------------------------------------------------
TEST_F(RGA2DTests, grid)
{
    RGA2DGrid grid(20, 10);
    const std::vector<RRect> rects{RRect(0, 0, 5, 5)};
    const RGA2DShape shape{&rects, 5, 5};
    EXPECT_TRUE(grid.fits(shape, {0, 0}));
    EXPECT_FALSE(grid.fits(shape, {16, 0}));
    grid.assign(rects, {0, 0}, 3);
    EXPECT_EQ(grid.objAt(4, 4), 3u);
    EXPECT_EQ(grid.objAt(5, 4), cNoRef);
    EXPECT_FALSE(grid.fits(shape, {4, 4}));
    EXPECT_TRUE(grid.fits(shape, {5, 0}));
    EXPECT_FALSE(grid.canMove(shape, {5, 0}, -1, 0));
    EXPECT_TRUE(grid.canMove(shape, {5, 0}, 1, 0));
    EXPECT_FALSE(grid.canMove(shape, {0, 5}, 0, 1));             // Top of the grid
    EXPECT_TRUE(grid.canMove(shape, {0, 5}, 1, 0));
    EXPECT_FALSE(grid.canMove(shape, {0, 5}, 0, -1));
    EXPECT_THROW(grid.assign(rects, {18, 0}, 1), RException);

    // Corners of a union of rectangles (an L shape has six corners)
    const auto corners = RGA2DCenterHeuristic::corners({RRect(0, 0, 20, 10), RRect(0, 10, 10, 20)});
    EXPECT_EQ(corners.size(), 6u);
}


//------------------------------------------------------------------------------
TEST_F(RGA2DTests, vlsiProblem)
{
    RVLSIProblem problem;
    problem.read(RJSON::parse(generate(1)));
    const auto& p = problem.problem();
    EXPECT_EQ(p.nbObjs(), 12u);
    EXPECT_EQ(p.nets().size(), 10u);
    EXPECT_EQ(p.boardConnectors().size(), 1u);
    EXPECT_EQ(problem.master(2), "L");
    EXPECT_EQ(p.obj(2).configs()[0].rects.size(), 2u);   // L shape

    // Invalid problems
    RVLSIProblem invalid;
    EXPECT_THROW(invalid.read(RJSON::parse(R"({"masters": [], "instances": []})")), RException);
    EXPECT_THROW(invalid.read(RJSON::parse(R"({"size": [10, 10], "masters": [], "instances": [{"id": "a", "master": "x"}]})")), RException);
    EXPECT_THROW(invalid.read(RJSON::parse(R"({"size": [10, 10], "masters": [{"id": "m", "polygon": [[0, 0], [5, 5], [0, 5]]}], "instances": []})")), RException);
    EXPECT_THROW(invalid.read(RJSON::parse(R"({"size": [10, 10], "masters": [{"id": "m", "polygon": [[0, 0], [5, 0], [5, 5], [0, 5]]}],
        "instances": [{"id": "a", "master": "m"}], "nets": [{"id": "n", "connect": [{"instance": "a", "terminal": "T"}]}]})")), RException);
}


//------------------------------------------------------------------------------
TEST_F(RGA2DTests, heuristics)
{
    RVLSIProblem problem;
    problem.read(RJSON::parse(generate(2)));
    for (const auto* h : {"bottom_left", "edge", "center"})
        for (const auto* ori : {"false", "true"})
        {
            const auto population = run(problem, std::string(R"("heuristic": ")") + h + R"(", "all_orientations": )" + ori, 1, 15);
            const auto& best = population->best();
            mLogInfo(logger, h, " (all orientations: ", ori, "): area ratio ", best.areaRatio(), ", length ", best.length());
            EXPECT_NO_THROW(best.verifyLayout(true));
            EXPECT_EQ(best.criteria().size(), 2u);
            EXPECT_GT(best.areaRatio(), 0.0);
            EXPECT_LE(best.areaRatio(), 1.0);
            for (std::size_t i = 0; i < population->nbChromos(); ++i)
                EXPECT_NO_THROW((*population)[i].verifyLayout(true));
        }
}


//------------------------------------------------------------------------------
TEST_F(RGA2DTests, blocks)
{
    // A block keeps the relative positions of its objects
    RVLSIProblem problem;
    problem.read(RJSON::parse(generate(3)));
    RGA2DLayout layout;
    layout.initLayout(problem.problem());
    layout.place(0, 0, {30, 40});
    layout.place(1, 0, {40, 50});
    const auto block = RGA2DBlock::build(layout, {0, 1});
    EXPECT_EQ(block.width, 40);
    EXPECT_EQ(block.height, 20);
    ASSERT_EQ(block.members.size(), 2u);
    EXPECT_EQ(block.members[1].pos, RPoint(10, 10));
    EXPECT_EQ(block.rects.size(), 2u);
}


//------------------------------------------------------------------------------
TEST_F(RGA2DTests, threads)
{
    // The result does not depend on the number of threads
    RVLSIProblem problem;
    problem.read(RJSON::parse(generate(4)));
    const auto pop1 = run(problem, R"("heuristic": "edge", "all_orientations": true)", 1, 8);
    const auto pop4 = run(problem, R"("heuristic": "edge", "all_orientations": true)", 4, 8);
    for (std::size_t i = 0; i < pop1->nbChromos(); ++i)
    {
        EXPECT_TRUE((*pop1)[i].isSame((*pop4)[i]));
        EXPECT_EQ((*pop1)[i].fitness(), (*pop4)[i].fitness());
    }
}
