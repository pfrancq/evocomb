/*

    EvoComb

    MatrixBasicTest.cpp

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
#include "matrix/r_matrix.hpp"
#include "matrix/r_symmetric_matrix.hpp"
using namespace evocomb;


//------------------------------------------------------------------------------
mLogModule("r.unit_tests");


//------------------------------------------------------------------------------
class RMatrixBasicTests : public RGoogleTest
{
};


//------------------------------------------------------------------------------
TEST_F(RMatrixBasicTests, vector)
{
    const RVector vec1({2.2, 3.4, 4.7});
    RVector vec2(3);
    vec2.init(1.0);
    auto vec3 = vec1 + vec2;
    ASSERT_TRUE(vec3[0] == 3.2);
    ASSERT_TRUE(vec3[1] == 4.4);
    ASSERT_TRUE(vec3[2] == 5.7);
    const RVector vec4({6.4, 8.8, 11.4});
    ASSERT_TRUE(vec4 == 2 * vec3);
}


//------------------------------------------------------------------------------
TEST_F(RMatrixBasicTests, vectorResize)
{
    // Enlarge: the old values are kept and the new ones are set to val
    RVector vec({1.0, 2.0, 3.0});
    vec.resize(5, 7.5);
    EXPECT_EQ(vec, RVector({1.0, 2.0, 3.0, 7.5, 7.5}));

    // Shrink: the first values are kept (val is not used)
    vec.resize(2, 9.0);
    EXPECT_EQ(vec, RVector({1.0, 2.0}));

    // Same size: nothing changes
    vec.resize(2, 9.0);
    EXPECT_EQ(vec, RVector({1.0, 2.0}));

    // Without a value, the new values are null
    vec.resize(3);
    EXPECT_EQ(vec, RVector({1.0, 2.0, 0.0}));
}


//------------------------------------------------------------------------------
TEST_F(RMatrixBasicTests, normal)
{
    RMatrix matrix({{2.2, 7.9}, {2.3, 9.0}, {4.5, 9.0}});
    ASSERT_TRUE(matrix(1,0) == 2.3);
    matrix *= 0.0;
    ASSERT_TRUE(matrix(1,0) == 0.0);
    const RMatrix matrix2({{2.2, 7.9}, {2.3, 9.0}, {4.5, 9.0}});
    matrix += matrix2;
    ASSERT_TRUE(matrix(1,0) == 2.3);
    const RMatrix matrix3({{2.9, 2.0, 1.2}, {9.0, 1.2, 3.2}, {8.1, 7.2, 2.1}, {9.1, 3.2, 2.3}});
    const auto matrix4 = matrix3 * (2 * matrix);
}

//------------------------------------------------------------------------------
TEST_F(RMatrixBasicTests, lower)
{
    RLowerTriangularMatrix matrix({{1}, {2, 3}, {3, 4, 5}});
    ASSERT_TRUE(matrix(1,0) == 2.0);
    ASSERT_TRUE(std::as_const(matrix)(0,1) == 0.0);
}

//------------------------------------------------------------------------------
TEST_F(RMatrixBasicTests, symmetric)
{
    RSymmetricMatrix matrix({{1}, {2, 3}, {3, 4, 5}});
    ASSERT_TRUE(matrix(1,0) == 2.0);
    ASSERT_TRUE(matrix(0,1) == 2.0);
}


//------------------------------------------------------------------------------
TEST_F(RMatrixBasicTests, complex)
{
    const RMatrix matrix({{2.2, 7.9, 1.0}, {2.3, 9.0, 2.1}, {4.5, 9.0, 2.0}});
    const RSymmetricMatrix matrix2({{1}, {2, 3}, {3, 4, 5}});
    const auto matrix3 = matrix + matrix2;
    ASSERT_TRUE(matrix3(2,2) == 7);
}