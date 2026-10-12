/*

    EvoComb

    ChoquetBasicTest.cpp

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
// include files for EvoComb
#include "r_google_test.hpp"
#include "multicriteria/r_2_additive_choquet.hpp"
using namespace evocomb;


//------------------------------------------------------------------------------
mLogModule("r.unit_tests");


//------------------------------------------------------------------------------
class RChoquetBasicTests : public RGoogleTest
{
};


//------------------------------------------------------------------------------
TEST_F(RChoquetBasicTests, test)
{
    // Create a two criteria case
    R2AdditiveChoquet choquet(2, "test");
    RSymmetricMatrix weights(2);
    weights(0,0) = 0.35;
    weights(1,0) = 0.30;
    weights(1,1) = 0.35;
    choquet.setParams(weights);
    ASSERT_TRUE(fabs(choquet.compute({2.1, 3.9}) - 1.83) < 0.0001);
}