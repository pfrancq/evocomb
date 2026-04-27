/*

    EvoComb

    RMaxTests.cpp

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
#include <vector>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "r_google_test.hpp"
#include "matrix/r_matrix.hpp"
#include "matrix/r_max_matrix.hpp"
#include "tools/r_exception.hpp"
using namespace evocomb;


//------------------------------------------------------------------------------
class RMaxTests : public RGoogleTest
{
protected:

    /**
     * Get the identifiers of a vector (in its order).
     */
    static std::vector<std::size_t> ids(const RMaxVector& vector)
    {
        std::vector<std::size_t> res;
        for (const auto& [id, value] : vector)
            res.push_back(id);
        return res;
    }
};


//------------------------------------------------------------------------------
TEST_F(RMaxTests, vector)
{
    RMaxVector a;
    EXPECT_TRUE(a.empty());
    EXPECT_EQ(a.add(1, 3.0), 0u);
    EXPECT_EQ(a.add(2, 5.0), 0u);
    EXPECT_EQ(a.add(3, 1.0), 2u);
    EXPECT_EQ(a.add(4, 3.0), 2u);         // After the equal value
    EXPECT_EQ(ids(a), (std::vector<std::size_t>{2, 1, 4, 3}));
    EXPECT_DOUBLE_EQ(a.max(), 5.0);
    EXPECT_DOUBLE_EQ(a.min(), 1.0);
    EXPECT_DOUBLE_EQ(a[1].value_, 3.0);
    EXPECT_DOUBLE_EQ(a.value(0), 5.0);

    // Update of an existing identifier
    EXPECT_EQ(a.add(3, 10.0), 0u);
    EXPECT_EQ(ids(a), (std::vector<std::size_t>{3, 2, 1, 4}));
    EXPECT_EQ(a.size(), 4u);

    // Search
    EXPECT_TRUE(a.contains(4));
    EXPECT_FALSE(a.contains(7));
    EXPECT_EQ(a.position(2), 1u);
    EXPECT_EQ(a.position(7), cNoRef);
    ASSERT_NE(a.find(1), nullptr);
    EXPECT_DOUBLE_EQ(a.find(1)->value_, 3.0);
    EXPECT_EQ(a.find(7), nullptr);
    EXPECT_DOUBLE_EQ(a.valueOf(7, -1.0), -1.0);

    // Computations
    EXPECT_DOUBLE_EQ(a.sum(), 21.0);
    EXPECT_DOUBLE_EQ(a.mean(), 5.25);
    EXPECT_EQ(a.countAtLeast(3.0), 4u);
    EXPECT_EQ(a.countAtLeast(4.0), 2u);
    EXPECT_EQ(a.countAtLeast(20.0), 0u);

    // Deletion
    EXPECT_TRUE(a.remove(2));
    EXPECT_FALSE(a.remove(2));
    EXPECT_EQ(ids(a), (std::vector<std::size_t>{3, 1, 4}));
    a.clear();
    EXPECT_TRUE(a.empty());
    EXPECT_DOUBLE_EQ(a.mean(), 0.0);
    EXPECT_THROW(static_cast<void>(a.max()), std::out_of_range);
}


//------------------------------------------------------------------------------
TEST_F(RMaxTests, vectorMaxSize)
{
    // Keep the 3 greatest values
    RMaxVector a(3, 7);
    EXPECT_EQ(a.id(), 7u);
    a.add(1, 1.0);
    a.add(2, 2.0);
    a.add(3, 3.0);
    EXPECT_EQ(a.add(4, 0.5), cNoRef);     // Too small: not inserted
    EXPECT_EQ(a.add(5, 2.5), 1u);         // The smallest one (1) is removed
    EXPECT_EQ(ids(a), (std::vector<std::size_t>{3, 5, 2}));
    EXPECT_EQ(a.add(2, 4.0), 0u);         // Update of an existing identifier
    EXPECT_EQ(ids(a), (std::vector<std::size_t>{2, 3, 5}));

    a.setMaxSize(2);
    EXPECT_EQ(ids(a), (std::vector<std::size_t>{2, 3}));

    // Copy and comparison
    const auto b = a;
    EXPECT_EQ(a, b);
    a.add(9, 10.0);
    EXPECT_NE(a, b);
}


//------------------------------------------------------------------------------
TEST_F(RMaxTests, matrix)
{
    // The 2 nearest neighbors of 3 objects
    RMaxMatrix sims(3, 2);
    EXPECT_EQ(sims.type(), RMatrixType::Max);
    sims.add(0, 1, 0.5);
    sims.add(0, 2, 0.9);
    sims.add(0, 1, 0.7);                  // Update
    sims.add(1, 0, 0.7);
    sims.add(1, 2, 0.2);
    sims.add(1, 3, 0.1);                  // Too small
    EXPECT_EQ(ids(sims[0]), (std::vector<std::size_t>{2, 1}));
    EXPECT_EQ(ids(sims[1]), (std::vector<std::size_t>{0, 2}));
    EXPECT_TRUE(sims[2].empty());
    EXPECT_EQ(sims[1].id(), 1u);

    // Generic access: j-th greatest value of line i (with a const reference, since the non-const operator() cannot be
    // used)
    const auto& csims = sims;
    EXPECT_DOUBLE_EQ(csims(0, 0), 0.9);
    EXPECT_DOUBLE_EQ(csims(0, 1), 0.7);
    EXPECT_DOUBLE_EQ(csims(2, 1), 0.0);
    EXPECT_THROW(static_cast<void>(sims(0, 0)), RException);

    // Generic operators
    const RMatrix sum = sims + sims;
    EXPECT_DOUBLE_EQ(sum(0, 0), 1.8);
    EXPECT_DOUBLE_EQ(sum(1, 1), 0.4);

    // Size
    sims.verifySize(4, 1);
    EXPECT_EQ(sims.nbLines(), 4u);
    EXPECT_EQ(sims.nbCols(), 1u);
    EXPECT_EQ(ids(sims[0]), std::vector<std::size_t>{2});
    EXPECT_EQ(sims[3].id(), 3u);
    sims.clear();
    EXPECT_TRUE(sims[0].empty());

    // Invalid operations
    EXPECT_THROW(static_cast<void>(csims(5, 0)), std::range_error);
    EXPECT_THROW(static_cast<void>(sims[5]), std::range_error);
    EXPECT_THROW(sims.add(5, 0, 1.0), std::range_error);
    EXPECT_THROW(sims.init(1.0), RException);
}
