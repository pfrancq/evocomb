/*

    EvoComb

    IOFileTests.cpp

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
#include <algorithm>
#include <cstring>
#include <random>
#include <vector>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "r_google_test.hpp"
#include "io/r_io.hpp"
#include "io/r_io_file.hpp"
using namespace evocomb;


//------------------------------------------------------------------------------
mLogModule("r.unit_tests");


//------------------------------------------------------------------------------
class RIOFileTests : public RGoogleTest
{
};


//------------------------------------------------------------------------------
TEST_F(RIOFileTests, modes)
{
    RSmartTempFile tmp;
    RIOFile file(tmp.path());

    // Create: the file is emptied and cannot be read
    file.open(RIO::Create);
    file.write("abcdef", 6);
    ASSERT_EQ(file.size(), 6);
    char buf[16];
    ASSERT_THROW(file.read(buf, 1), RIOException);
    file.close();

    // Read: the file cannot be written, and a position beyond the end is refused
    file.open(RIO::Read);
    ASSERT_EQ(file.read(buf, 16), 6u);
    ASSERT_EQ(std::memcmp(buf, "abcdef", 6), 0);
    ASSERT_TRUE(file.end());
    ASSERT_THROW(file.read(buf, 1), RIOException);
    ASSERT_THROW(file.write("x", 1), RIOException);
    ASSERT_THROW(file.seek(7), RIOException);
    ASSERT_THROW(file.seekRel(-7), RIOException);
    file.close();

    // Write: the file is not emptied
    file.open(RIO::Write);
    ASSERT_EQ(file.size(), 6);
    file.seek(2);
    file.write("XY", 2);
    file.close();

    // Append: every write is at the end
    file.open(RIO::Append);
    ASSERT_EQ(file.pos(), 6);
    file.seek(0);
    file.write("gh", 2);
    ASSERT_EQ(file.pos(), 8);
    file.seek(0);
    ASSERT_EQ(file.read(buf, 16), 8u);
    ASSERT_EQ(std::memcmp(buf, "abXYefgh", 8), 0);
    file.close();

    // Read of a missing file
    RIOFile missing(tmp.path().string() + ".missing");
    ASSERT_THROW(missing.open(RIO::Read), RIOException);
}


//------------------------------------------------------------------------------
TEST_F(RIOFileTests, peekAndTruncate)
{
    RSmartTempFile tmp;
    RIOFile file(tmp.path());
    file.open(RIO::ReadWrite);
    file.write("0123456789", 10);

    // Read without moving
    file.seek(3);
    char buf[4];
    ASSERT_EQ(file.read(buf, 2, false), 2u);
    ASSERT_EQ(file.pos(), 3);
    ASSERT_EQ(file.read(buf, 2), 2u);
    ASSERT_EQ(std::memcmp(buf, "34", 2), 0);
    ASSERT_EQ(file.pos(), 5);

    // Truncate: the position is moved to the new end
    file.seekToEnd();
    file.truncate(4);
    ASSERT_EQ(file.size(), 4);
    ASSERT_EQ(file.pos(), 4);
    ASSERT_TRUE(file.end());
    file.seek(0);
    ASSERT_EQ(file.read(buf, 4), 4u);
    ASSERT_EQ(std::memcmp(buf, "0123", 4), 0);

    // Write beyond the end: the gap is filled with 0
    file.seek(6);
    file.write("Z", 1);
    ASSERT_EQ(file.size(), 7);
    file.seek(4);
    ASSERT_EQ(file.read(buf, 3), 3u);
    ASSERT_EQ(std::memcmp(buf, "\0\0Z", 3), 0);
}


//------------------------------------------------------------------------------
/**
 * Random reads, writes and seeks are applied to a file and to a model (a vector of bytes), and the results are
 * compared.
 */
TEST_F(RIOFileTests, randomOperations)
{
    for (unsigned seed = 1; seed <= 5; ++seed)
    {
        std::mt19937 random(seed);
        RSmartTempFile tmp;
        RIOFile file(tmp.path());
        file.open(RIO::ReadWrite);
        std::vector<char> model;
        std::streamoff pos = 0;
        std::vector<char> buf(40000);

        for (int i = 0; i < 3000; ++i)
        {
            const auto big = random() % 2 == 0;
            switch (random() % 4)
            {
                case 0:
                {
                    const std::size_t nb = random() % (big ? 30000 : 200);
                    for (std::size_t k = 0; k < nb; ++k)
                        buf[k] = static_cast<char>(random());
                    file.write(buf.data(), nb);
                    if (model.size() < static_cast<std::size_t>(pos) + nb)
                        model.resize(static_cast<std::size_t>(pos) + nb);
                    std::copy_n(buf.begin(), nb, model.begin() + pos);
                    pos += static_cast<std::streamoff>(nb);
                    break;
                }
                case 1:
                {
                    if (pos >= static_cast<std::streamoff>(model.size()))
                        break;
                    const std::size_t nb = random() % (big ? 25000 : 300) + 1;
                    const bool move = random() % 3 != 0;
                    const auto read = file.read(buf.data(), nb, move);
                    const auto expected = std::min(nb, model.size() - static_cast<std::size_t>(pos));
                    ASSERT_EQ(read, expected) << "seed " << seed << ", operation " << i;
                    ASSERT_TRUE(std::equal(buf.begin(), buf.begin() + static_cast<std::ptrdiff_t>(read), model.begin() + pos))
                        << "seed " << seed << ", operation " << i;
                    if (move)
                        pos += static_cast<std::streamoff>(read);
                    break;
                }
                case 2:
                    if (!model.empty())
                    {
                        pos = static_cast<std::streamoff>(random() % model.size());
                        file.seek(pos);
                    }
                    break;
                default:
                {
                    const auto rel = static_cast<std::streamoff>(random() % 200) - 100;
                    if (pos + rel >= 0 && pos + rel <= static_cast<std::streamoff>(model.size()))
                    {
                        file.seekRel(rel);
                        pos += rel;
                    }
                    break;
                }
            }
            ASSERT_EQ(file.pos(), pos) << "seed " << seed << ", operation " << i;
            ASSERT_EQ(file.size(), static_cast<std::streamoff>(model.size())) << "seed " << seed << ", operation " << i;
        }

        // The content of the file on the disk
        file.close();
        file.open(RIO::Read);
        std::vector<char> content(model.size());
        if (!content.empty())
            file.read(content.data(), content.size());
        ASSERT_EQ(content, model) << "seed " << seed;
    }
}
