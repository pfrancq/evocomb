/*

    EvoComb

    PrometheeBasicTest.cpp

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
#include <memory>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "r_google_test.hpp"
#include "multicriteria/r_prom_kernel.hpp"
#include "tools/r_exception.hpp"
#include "tools/r_json.hpp"
using namespace evocomb;


//------------------------------------------------------------------------------
mLogModule("r.unit_tests");


//------------------------------------------------------------------------------
class RPrometheeBasicTests : public RGoogleTest
{
};


//------------------------------------------------------------------------------
TEST_F(RPrometheeBasicTests, test)
{
    RPromKernel kernel(logger, "test");
    kernel.addCriterion(std::make_unique<RPromLinearCriterion>(RCriteriaType::Minimize, 0.2, 0.05, 1.0, "price"));
    kernel.addCriterion(std::make_unique<RPromLinearCriterion>(RCriteriaType::Minimize, 0.2, 0.05, 1.0, "cons"));
    kernel.addCriterion(std::make_unique<RPromLinearCriterion>(RCriteriaType::Maximize, 0.2, 0.05, 1.0, "confort"));
    kernel.addCriterion(std::make_unique<RPromLinearCriterion>(RCriteriaType::Maximize, 0.2, 0.05, 1.0, "power"));
    kernel.createSolutions({"car1", "car2", "car3", "car4"});
    kernel.assign(0, {8.75, 6.2, 1, 30});
    kernel.assign(1, {13.75, 7.5, 1, 50});
    kernel.assign(2, {25, 8, 3, 80});
    kernel.assign(3, {62.5, 20, 2, 120});
    kernel.computePrometheeII();
    ASSERT_TRUE(kernel.bestSolution().index() == 2);
}


//------------------------------------------------------------------------------
TEST_F(RPrometheeBasicTests, json)
{
    // Same problem, with the criteria defined in JSON
    RPromKernel kernel(logger, "test");
    kernel.addCriteria(RJSON::parse(R"([
        {"name": "price", "type": "minimize", "p": 0.2, "q": 0.05, "weight": 1},
        {"name": "cons", "type": "minimize"},
        {"name": "confort", "type": "maximize", "weight": 1.0},
        {"name": "power", "type": "maximize"}
    ])"));
    EXPECT_EQ(kernel.nbCriteria(), 4u);
    kernel.createSolutions({"car1", "car2", "car3", "car4"});
    kernel.assign(0, {8.75, 6.2, 1, 30});
    kernel.assign(1, {13.75, 7.5, 1, 50});
    kernel.assign(2, {25, 8, 3, 80});
    kernel.assign(3, {62.5, 20, 2, 120});
    kernel.computePrometheeII();
    EXPECT_EQ(kernel.bestSolution().index(), 2u);

    // The Fi are in [-1, 1]
    for (const auto sol : kernel.solutions())
    {
        EXPECT_GE(sol->fi(), -1.0);
        EXPECT_LE(sol->fi(), 1.0);
    }

    // Invalid descriptions (nothing is added)
    RPromKernel kernel2(logger, "test2");
    EXPECT_THROW(kernel2.addCriteria(RJSON::parse(R"({"type": "maximize"})")), RException);
    EXPECT_THROW(kernel2.addCriteria(RJSON::parse(R"([{"type": "maximize"}, {"type": "best"}])")), RException);
    EXPECT_THROW(kernel2.addCriteria(RJSON::parse(R"([{"type": "maximize", "weight": 0}])")), RException);
    EXPECT_EQ(kernel2.nbCriteria(), 0u);
}
