/*

    EvoComb

    IOBlockFileTests.cpp

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
#include <cstring>
#include <map>
#include <random>
#include <string>
#include <utility>
#include <vector>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "r_google_test.hpp"
#include "io/r_io.hpp"
#include "io/r_block_file.hpp"
#include "io/r_key_value_file.hpp"
#include "io/r_keys.hpp"
using namespace evocomb;


//------------------------------------------------------------------------------
mLogModule("r.unit_tests");


//------------------------------------------------------------------------------
class RBlockFileTests : public RGoogleTest
{
};


//------------------------------------------------------------------------------
TEST_F(RBlockFileTests, blocks)
{
    RSmartTempFile tmp;
    {
        RBlockFile file(tmp.path(), 1024, 2);
        file.open();
        ASSERT_EQ(file.nbBlocks(), 0u);
        file.seek(2, 20);
        file.write("hello", 5);
        file.seek(3, 10);
        file.write("world!", 6);
        file.seek(1, 1020);
        ASSERT_THROW(file.write("12345", 5), RIOException);      // Crosses the end of the block
        file.write("1234", 4);
        ASSERT_EQ(file.nbBlocks(), 3u);
        file.moveBlock(3, 10, 12, 6);                            // "world!" is moved 2 bytes further
        ASSERT_EQ(file.currentPos(), 18u);
    }

    // Reopen the file: the block size is read from the file
    RBlockFile file(tmp.path(), 1);
    file.open();
    ASSERT_EQ(file.blockSize(), 1024u);
    ASSERT_EQ(file.nbBlocks(), 3u);
    char buf[8];
    file.seek(2, 20);
    file.read(buf, 5);
    ASSERT_EQ(std::string(buf, 5), "hello");
    file.seek(3, 12);
    file.read(buf, 6);
    ASSERT_EQ(std::string(buf, 6), "world!");
    file.seek(1, 1020);
    ASSERT_EQ(std::string(file.getPtr(4), 4), "1234");
    file.seekRel(-4);
    ASSERT_EQ(file.currentPos(), 1020u);
    file.close();

    // Another block size is refused
    RBlockFile other(tmp.path(), 2048, 2);
    ASSERT_THROW(other.open(), RIOException);
}


//------------------------------------------------------------------------------
TEST_F(RBlockFileTests, keyValue)
{
    RSmartTempFile tmp;
    RKeyValueFile<RIntKey> file(tmp.path(), 1024, 2, 100);
    file.open();

    std::size_t block = 0;
    file.write(block, RIntKey(300), std::vector<std::uint32_t>{3, 6, 7, 10, 11});
    ASSERT_EQ(block, 1u);
    std::vector<std::uint32_t> values;
    file.read(block, RIntKey(300), values);
    ASSERT_EQ(values, (std::vector<std::uint32_t>{3, 6, 7, 10, 11}));
    ASSERT_THROW(file.read(block, RIntKey(301), values), RIOException);
    ASSERT_THROW(file.read(block, RIntKey(300), *std::make_unique<std::vector<double>>()), RIOException);

    // A read cannot cross the end of the value
    file.seek(block, RIntKey(300));
    char buf[64];
    ASSERT_THROW(file.read(buf, file.recordSize() + 1), RIOException);

    // Too large value
    std::size_t other = 0;
    ASSERT_THROW(file.seek(other, RIntKey(1), 1024), RIOException);

    // Keys of two integers
    RSmartTempFile tmp2;
    RKeyValueFile<RIntsKey> file2(tmp2.path(), 512, 1, 0);
    file2.open();
    std::size_t b1 = 0, b2 = 0;
    file2.write(b1, RIntsKey(1, 2), std::vector<double>{1.5});
    file2.write(b2, RIntsKey(1, 1), std::vector<double>{2.5, 3.5});
    ASSERT_EQ(b1, b2);
    file2.read(b2, RIntsKey(1, 1), *std::make_unique<std::vector<double>>());
    ASSERT_TRUE(file2.exists(b1, RIntsKey(1, 2)));
    file2.erase(b1, RIntsKey(1, 2));
    ASSERT_FALSE(file2.exists(b1, RIntsKey(1, 2)));
    ASSERT_TRUE(file2.exists(b1, RIntsKey(1, 1)));
}


//------------------------------------------------------------------------------
/**
 * Random writes, rewrites, erasures and reads of pairs, compared with a model. The file is regularly closed and open
 * again, and the cache is small to force the replacement of blocks.
 */
TEST_F(RBlockFileTests, keyValueRandom)
{
    for (unsigned seed = 1; seed <= 4; ++seed)
    {
        std::mt19937 random(seed);
        RSmartTempFile tmp;
        RKeyValueFile<RIntKey> file(tmp.path(), 1024, 2, 64);
        file.open();
        if (seed % 2 == 0)
            file.cacheType(RKeyValueFile<RIntKey>::CacheType::WriteThrough);
        std::map<std::uint64_t, std::pair<std::size_t, std::string>> model;     // key -> (block, value)

        for (int i = 0; i < 4000; ++i)
        {
            const auto key = static_cast<std::uint64_t>(random() % 150);
            const auto it = model.find(key);
            const auto op = random() % 10;
            if (op < 5)
            {
                // Write (a new pair or a new value for an existing pair)
                std::string value(random() % 300, ' ');
                for (auto& c : value)
                    c = static_cast<char>('a' + random() % 26);
                std::size_t block = (it == model.end()) ? 0 : it->second.first;
                file.seek(block, RIntKey(key), value.size());
                file.write(value.data(), value.size());
                model[key] = {block, value};
            }
            else if (op < 7 && it != model.end())
            {
                file.erase(it->second.first, RIntKey(key));
                model.erase(it);
            }
            else if (op < 9 && it != model.end())
            {
                file.seek(it->second.first, RIntKey(key));
                ASSERT_EQ(file.recordSize(), it->second.second.size()) << "seed " << seed << ", operation " << i;
                std::string value(file.recordSize(), 0);
                file.read(value.data(), value.size());
                ASSERT_EQ(value, it->second.second) << "seed " << seed << ", operation " << i;
            }
            else if (op == 9 && random() % 20 == 0)
            {
                file.close();
                file.open();
            }
        }

        // Verify all the pairs (the random operations must have used several blocks)
        ASSERT_GT(file.nbBlocks(), 5u);
        file.close();
        file.open();
        for (const auto& [key, pair] : model)
        {
            file.seek(pair.first, RIntKey(key));
            std::string value(file.recordSize(), 0);
            file.read(value.data(), value.size());
            ASSERT_EQ(value, pair.second) << "seed " << seed << ", key " << key;
        }
    }
}
