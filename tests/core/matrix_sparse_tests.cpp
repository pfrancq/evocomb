/*

    EvoComb

    RSparseTests.cpp

    Tests of the sparse vectors and matrices.

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
#include <stdexcept>
#include <utility>
#include <vector>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "r_google_test.hpp"
#include "matrix/r_matrix.hpp"
#include "matrix/r_sparse_symmetric_matrix.hpp"
using namespace evocomb;


//------------------------------------------------------------------------------
class RSparseTests : public RGoogleTest
{
protected:

    /**
     * Get the indexes of the elements of a vector (in their order).
     */
    static std::vector<std::size_t> ids(const RSparseVector& vector)
    {
        std::vector<std::size_t> res;
        for (const auto& [id, value] : vector)
            res.push_back(id);
        return res;
    }
};


//------------------------------------------------------------------------------
TEST_F(RSparseTests, vector)
{
    RSparseVector a(3);
    EXPECT_EQ(a.id(), 3u);
    a.set(15, 3.0);
    a[5] = 2.0;
    a.set(0, 1.0);
    a.add(5, 0.5);
    EXPECT_EQ(a.size(), 3u);
    EXPECT_EQ(ids(a), (std::vector<std::size_t>{0, 5, 15}));
    EXPECT_DOUBLE_EQ(std::as_const(a)[5], 2.5);
    EXPECT_DOUBLE_EQ(std::as_const(a)[4], 0.0);
    EXPECT_EQ(a.size(), 3u);                            // Const access creates nothing
    EXPECT_TRUE(a.contains(15));
    EXPECT_EQ(a.find(4), nullptr);

    // Removal
    EXPECT_TRUE(a.remove(5));
    EXPECT_FALSE(a.remove(5));
    a[7] = 0.0;
    a.removeNulls();
    EXPECT_EQ(ids(a), (std::vector<std::size_t>{0, 15}));
    a.truncate(10);
    EXPECT_EQ(ids(a), (std::vector<std::size_t>{0}));
    a.setExisting(4.0);
    EXPECT_DOUBLE_EQ(std::as_const(a)[0], 4.0);
}


//------------------------------------------------------------------------------
TEST_F(RSparseTests, matrix)
{
    RSparseMatrix a(6, 6);
    EXPECT_EQ(a.type(), RMatrixType::Sparse);
    a(0, 2) = 0.2;
    a.set(1, 5, 1.5);
    a.add(2, 0, 1.0);
    a.add(2, 0, 1.0);
    EXPECT_DOUBLE_EQ(std::as_const(a)(0, 2), 0.2);
    EXPECT_DOUBLE_EQ(std::as_const(a)(2, 0), 2.0);
    EXPECT_DOUBLE_EQ(std::as_const(a)(3, 3), 0.0);
    EXPECT_EQ(a.nbValues(), 3u);
    EXPECT_EQ(ids(a[1]), (std::vector<std::size_t>{5}));
    EXPECT_THROW(static_cast<void>(std::as_const(a)(6, 0)), std::range_error);
    EXPECT_THROW(a.set(0, 6, 1.0), std::range_error);

    // Generic operations
    const RMatrix sum = a + a;
    EXPECT_DOUBLE_EQ(sum(1, 5), 3.0);

    // Size
    a.verifySize(3, 5);
    EXPECT_EQ(a.nbValues(), 2u);                        // (1,5) is removed
    a.verifySize(8, 8);
    EXPECT_DOUBLE_EQ(std::as_const(a)(7, 7), 0.0);

    // Copy and comparison
    RSparseMatrix b(a);
    EXPECT_EQ(a, b);
    b.set(7, 7, 1.0);
    EXPECT_NE(a, b);
    b.set(7, 7, 0.0);
    b.removeNulls();
    EXPECT_EQ(a, b);

    // Initialization
    a.init(0.0);
    EXPECT_EQ(a.nbValues(), 0u);
    a.init(1.0);
    EXPECT_EQ(a.nbValues(), 64u);
    a.clear();
    EXPECT_EQ(a.nbValues(), 0u);
}


//------------------------------------------------------------------------------
TEST_F(RSparseTests, symmetric)
{
    RSparseSymmetricMatrix a(4);
    EXPECT_EQ(a.type(), RMatrixType::SparseSymmetric);
    a(0, 2) = 0.2;
    a.add(2, 0, 0.1);
    EXPECT_DOUBLE_EQ(std::as_const(a)(2, 0), 0.3);
    EXPECT_DOUBLE_EQ(std::as_const(a)(0, 2), 0.3);
    EXPECT_EQ(a.nbValues(), 1u);
    EXPECT_EQ(ids(a[2]), (std::vector<std::size_t>{0}));  // Lower part
    EXPECT_THROW(a.verifySize(4, 5), std::range_error);
    a.verifySize(2, 2);
    EXPECT_EQ(a.nbValues(), 0u);
    a.init(1.0);
    EXPECT_EQ(a.nbValues(), 3u);                        // (0,0), (1,0) and (1,1)
}
