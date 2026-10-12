/*

    EvoComb

    RMatrixStorageTests.cpp

    Tests of the matrix storage.

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
#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <utility>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "r_google_test.hpp"
#include "io/r_io.hpp"
#include "matrix/r_matrix.hpp"
#include "matrix/r_matrix_storage.hpp"
#include "matrix/r_max_matrix.hpp"
#include "matrix/r_sparse_symmetric_matrix.hpp"
#include "matrix/r_symmetric_matrix.hpp"
using namespace evocomb;


//------------------------------------------------------------------------------
class RMatrixStorageTests : public RGoogleTest
{
protected:

    /**
     * Create a temporary directory for the files of the storages.
     */
    void SetUp() override
    {
        RGoogleTest::SetUp();
        dir_ = std::filesystem::temp_directory_path() / ("evocomb_storage_" + std::to_string(reinterpret_cast<std::uintptr_t>(this)));
        std::filesystem::create_directories(dir_);
    }

    /**
     * Remove the temporary directory.
     */
    void TearDown() override
    {
        std::filesystem::remove_all(dir_);
        RGoogleTest::TearDown();
    }

    /**
     * Get the base path of a storage.
     * @param name           Name.
     */
    [[nodiscard]] std::filesystem::path base(const char* name) const { return dir_ / name; }

    std::filesystem::path dir_;         ///< Temporary directory.
};


//------------------------------------------------------------------------------
TEST_F(RMatrixStorageTests, normal)
{
    RMatrixStorage storage;
    storage.open(base("normal"), RMatrixType::Normal);
    EXPECT_EQ(storage.nbLines(), 0u);
    storage.verifySize(2, 3, true, 1.0);
    storage.write(1, 2, 5.0);
    EXPECT_DOUBLE_EQ(storage.read(1, 2), 5.0);
    EXPECT_DOUBLE_EQ(storage.read(0, 0), 1.0);
    EXPECT_THROW(static_cast<void>(storage.read(2, 0)), std::range_error);

    // Add lines and columns (the file is rewritten), then remove some
    storage.verifySize(3, 5, true, 2.0);
    EXPECT_DOUBLE_EQ(storage.read(1, 2), 5.0);
    EXPECT_DOUBLE_EQ(storage.read(1, 4), 2.0);
    EXPECT_DOUBLE_EQ(storage.read(2, 0), 2.0);
    storage.verifySize(2, 2);
    storage.verifySize(2, 4);                           // New columns are null
    EXPECT_DOUBLE_EQ(storage.read(1, 2), 0.0);
    EXPECT_DOUBLE_EQ(storage.read(1, 1), 1.0);

    // Persistence
    storage.close();
    storage.open(base("normal"), RMatrixType::Normal);
    EXPECT_EQ(storage.nbLines(), 2u);
    EXPECT_EQ(storage.nbCols(), 4u);
    RMatrix m(1, 1);
    storage.load(m);
    EXPECT_EQ(m.nbCols(), 4u);
    EXPECT_DOUBLE_EQ(m(1, 1), 1.0);

    // Save a matrix
    const RMatrix a({{1, 2, 3}, {4, 5, 6}});
    storage.save(a);
    EXPECT_EQ(storage.nbCols(), 3u);
    EXPECT_DOUBLE_EQ(storage.read(1, 0), 4.0);
    storage.initLine(0, 9.0);
    storage.initCol(2, 7.0);
    EXPECT_DOUBLE_EQ(storage.read(0, 1), 9.0);
    EXPECT_DOUBLE_EQ(storage.read(1, 2), 7.0);
    storage.clear(0.5);
    EXPECT_DOUBLE_EQ(storage.read(1, 1), 0.5);

    // Another type is refused
    storage.close();
    RMatrixStorage other;
    EXPECT_THROW(other.open(base("normal"), RMatrixType::Sparse), RIOException);
}


//------------------------------------------------------------------------------
TEST_F(RMatrixStorageTests, symmetric)
{
    RMatrixStorage storage;
    storage.open(base("sym"), RMatrixType::Symmetric);
    EXPECT_THROW(storage.verifySize(3, 4), std::range_error);
    storage.verifySize(3, 3, true, 0.0);
    storage.write(0, 2, 0.8);
    EXPECT_DOUBLE_EQ(storage.read(2, 0), 0.8);
    storage.verifySize(4, 4, true, 1.0);                // Only appends elements
    EXPECT_DOUBLE_EQ(storage.read(0, 2), 0.8);
    EXPECT_DOUBLE_EQ(storage.read(3, 1), 1.0);

    RSymmetricMatrix m(1);
    storage.load(m);
    EXPECT_EQ(m.nbLines(), 4u);
    EXPECT_DOUBLE_EQ(m(0, 2), 0.8);
    RMatrix wrong(4, 4);
    EXPECT_THROW(storage.load(wrong), RIOException);

    // Lower triangular
    RMatrixStorage lower;
    lower.open(base("lower"), RMatrixType::LowerTriangular);
    lower.verifySize(2, 2, true, 1.0);
    EXPECT_DOUBLE_EQ(lower.read(0, 1), 0.0);
    EXPECT_THROW(lower.write(0, 1, 1.0), std::range_error);
}


//------------------------------------------------------------------------------
TEST_F(RMatrixStorageTests, sparse)
{
    RMatrixStorage storage;
    storage.open(base("sparse"), RMatrixType::Sparse);
    storage.verifySize(4, 5);
    storage.write(1, 3, 1.5);
    storage.write(1, 0, 2.0);
    storage.write(3, 4, 3.0);
    storage.write(1, 3, 2.5);                           // Rewrite
    storage.write(2, 2, 0.0);                           // Nothing stored
    EXPECT_DOUBLE_EQ(storage.read(1, 3), 2.5);
    EXPECT_DOUBLE_EQ(storage.read(1, 0), 2.0);
    EXPECT_DOUBLE_EQ(storage.read(2, 2), 0.0);

    // Load
    RSparseMatrix m(1, 1);
    storage.load(m);
    EXPECT_EQ(m.nbValues(), 3u);
    EXPECT_DOUBLE_EQ(std::as_const(m)(1, 3), 2.5);
    EXPECT_EQ(m[1].values().front().id_, 0u);            // Ordered by column

    // Reduce the size: (3,4) is removed
    storage.write(1, 0, 0.0);
    storage.verifySize(4, 4);
    EXPECT_DOUBLE_EQ(storage.read(1, 3), 2.5);
    EXPECT_DOUBLE_EQ(storage.read(1, 0), 0.0);
    storage.verifySize(6, 6);
    storage.write(5, 5, 1.0);
    storage.compact();
    storage.close();
    storage.open(base("sparse"), RMatrixType::Sparse);
    storage.load(m);
    EXPECT_EQ(m.nbLines(), 6u);
    EXPECT_EQ(m.nbValues(), 2u);
    EXPECT_DOUBLE_EQ(std::as_const(m)(5, 5), 1.0);

    // Save
    RSparseMatrix a(3, 3);
    a.set(0, 1, 1.0);
    a.set(2, 2, 2.0);
    storage.save(a);
    EXPECT_EQ(storage.nbLines(), 3u);
    EXPECT_DOUBLE_EQ(storage.read(2, 2), 2.0);
    EXPECT_DOUBLE_EQ(storage.read(1, 1), 0.0);
    storage.write(1, 1, 4.0);
    storage.load(m);
    EXPECT_EQ(m.nbValues(), 3u);
    storage.init(1.0);
    EXPECT_DOUBLE_EQ(storage.read(1, 2), 1.0);
    storage.clear();
    EXPECT_DOUBLE_EQ(storage.read(1, 2), 0.0);

    // Symmetric
    RMatrixStorage sym;
    sym.open(base("sparse_sym"), RMatrixType::SparseSymmetric);
    RSparseSymmetricMatrix s(3);
    s.set(0, 2, 0.7);
    sym.save(s);
    EXPECT_DOUBLE_EQ(sym.read(0, 2), 0.7);
    sym.write(1, 2, 0.4);
    RSparseSymmetricMatrix s2(1);
    sym.load(s2);
    EXPECT_DOUBLE_EQ(std::as_const(s2)(2, 1), 0.4);
}


//------------------------------------------------------------------------------
TEST_F(RMatrixStorageTests, max)
{
    RMatrixStorage storage;
    storage.open(base("max"), RMatrixType::Max);
    storage.verifySize(2, 3);
    storage.write(0, 10, 0.5);
    storage.write(0, 11, 0.9);
    storage.write(0, 12, 0.7);
    storage.write(0, 13, 0.1);                          // Line full and too small
    EXPECT_EQ(storage.nbCols(0), 3u);
    EXPECT_DOUBLE_EQ(storage.read(0, 0), 0.9);
    EXPECT_DOUBLE_EQ(storage.read(0, 2), 0.5);
    storage.write(0, 13, 0.8);                          // Replaces 0.5
    storage.write(0, 11, 0.2);                          // Moves identifier 11
    EXPECT_DOUBLE_EQ(storage.read(0, 0), 0.8);
    EXPECT_DOUBLE_EQ(storage.read(0, 2), 0.2);
    EXPECT_DOUBLE_EQ(storage.read(1, 0), 0.0);
    EXPECT_THROW(storage.init(1.0), RException);

    // Load
    RMaxMatrix m(1, 1);
    storage.load(m);
    EXPECT_EQ(m.nbCols(), 3u);
    EXPECT_EQ(m[0][0].id_, 13u);
    EXPECT_EQ(m[0][2].id_, 11u);

    // Reduce the number of values
    storage.verifySize(3, 2);
    EXPECT_EQ(storage.nbCols(0), 2u);
    EXPECT_DOUBLE_EQ(storage.read(0, 1), 0.7);

    // Save a matrix without limit
    RMaxMatrix a(2, 0);
    for (std::size_t id = 0; id < 5; id++)
        a.add(1, id, static_cast<double>(id));
    storage.save(a);
    EXPECT_EQ(storage.nbCols(), 5u);
    EXPECT_DOUBLE_EQ(storage.read(1, 0), 4.0);
}


//------------------------------------------------------------------------------
TEST_F(RMatrixStorageTests, info)
{
    RMatrixStorage storage;
    storage.open(base("info"), RMatrixType::Sparse);
    EXPECT_FALSE(storage.hasInfo());
    storage.writeInfo(8, 3.5);
    storage.writeInfo(0, std::uint64_t{42});
    storage.verifySize(10, 10);                         // Information is kept
    EXPECT_EQ(storage.infoSize(), 16u);
    EXPECT_EQ(storage.readInfo<std::uint64_t>(0), 42u);
    EXPECT_DOUBLE_EQ(storage.readInfo<double>(8), 3.5);
    EXPECT_THROW(static_cast<void>(storage.readInfo<double>(12)), RIOException);
}
