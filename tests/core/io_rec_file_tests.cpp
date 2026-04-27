/*

    EvoComb

    IORecFileTests.cpp

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
#include <string>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "r_google_test.hpp"
#include "io/r_io.hpp"
#include "io/r_binary_file.hpp"
#include "io/r_rec_file.hpp"
using namespace evocomb;


//------------------------------------------------------------------------------
mLogModule("r.unit_tests");


//------------------------------------------------------------------------------
class RRecFileTests : public RGoogleTest
{
};


//------------------------------------------------------------------------------
/**
 * Record of a person identified by a number (40 bytes are reserved for the name).
 */
class Person
{
public:
    std::uint32_t id = 0;
    std::string name;

    Person() = default;
    Person(const std::uint32_t i, std::string n) : id(i), name(std::move(n)) {}
    void read(RRecFile<Person, true>& file) { file >> id >> name; }
    void write(RRecFile<Person, true>& file) const { file << id << name; }
    [[nodiscard]] int compare(const std::uint32_t tag) const { return (id < tag) ? -1 : (id > tag ? 1 : 0); }
};


//------------------------------------------------------------------------------
TEST_F(RRecFileTests, binary)
{
    enum class Color : std::uint8_t { Red, Green };
    RSmartTempFile tmp;
    RBinaryFile file(tmp.path());
    file.open(RIO::Create);
    file << true << 'x' << std::int32_t(-7) << std::uint64_t(1) << 3.25 << 1.5f << Color::Green << "abc" << std::string();
    file.close();

    bool b = false;
    char c = 0;
    std::int32_t i = 0;
    std::uint64_t u = 0;
    double d = 0;
    float f = 0;
    Color color = Color::Red;
    std::string s1, s2("not empty");
    file.open(RIO::Read);
    file >> b >> c >> i >> u >> d >> f >> color >> s1 >> s2;
    ASSERT_TRUE(b);
    ASSERT_EQ(c, 'x');
    ASSERT_EQ(i, -7);
    ASSERT_EQ(u, 1u);
    ASSERT_EQ(d, 3.25);
    ASSERT_EQ(f, 1.5f);
    ASSERT_EQ(color, Color::Green);
    ASSERT_EQ(s1, "abc");
    ASSERT_TRUE(s2.empty());
    ASSERT_TRUE(file.end());
    ASSERT_THROW(file >> i, RIOException);
}


//------------------------------------------------------------------------------
TEST_F(RRecFileTests, records)
{
    constexpr std::size_t recSize = sizeof(std::uint32_t) + sizeof(std::uint64_t) + 40;
    RSmartTempFile tmp;
    RRecFile<Person, true> file(tmp.path(), recSize);

    // Write ordered records
    file.open(RIO::Create);
    for (const auto& p : {Person(2, "Jagger"), Person(4, "Gillan"), Person(6, "Plant"), Person(8, "Coverdale")})
        file.writeRec(p);
    ASSERT_EQ(file.recNb(), 4u);
    ASSERT_EQ(file.size(), static_cast<std::streamoff>(4 * recSize));
    file.close();

    // Read them sequentially
    file.open(RIO::Read);
    std::string names;
    for (file.start(); !file.end(); file.next())
        names += file()->name + ";";
    ASSERT_EQ(names, "Jagger;Gillan;Plant;Coverdale;");
    ASSERT_EQ(file(), nullptr);

    // Direct access and reading with readRec
    file.goToRec(2);
    ASSERT_EQ(file.currentRec(), 2u);
    ASSERT_EQ(file()->name, "Plant");
    Person p;
    file.readRec(p);
    ASSERT_EQ(p.id, 6u);
    ASSERT_EQ(file.currentRec(), 3u);
    file.prev();
    ASSERT_EQ(file()->id, 6u);

    // Binary search: found, and insertion points
    ASSERT_EQ(file.search(4u), 1u);
    ASSERT_TRUE(file.found());
    ASSERT_EQ(file()->name, "Gillan");
    ASSERT_EQ(file.search(1u), 0u);
    ASSERT_FALSE(file.found());
    ASSERT_EQ(file.search(5u), 2u);
    ASSERT_FALSE(file.found());
    ASSERT_EQ(file.currentRec(), 2u);
    ASSERT_EQ(file.search(9u), 4u);
    ASSERT_FALSE(file.found());
    ASSERT_EQ(file(), nullptr);
    file.close();

    // Replace a record and append another one
    file.open(RIO::ReadWrite);
    file.goToRec(1);
    file.writeRec(Person(4, "Gillian"));
    file.close();
    file.open(RIO::Append);
    file.writeRec(Person(10, "Lord"));
    ASSERT_EQ(file.recNb(), 5u);
    file.goToRec(1);
    ASSERT_EQ(file()->name, "Gillian");
    file.goToRec(4);
    ASSERT_EQ(file()->name, "Lord");

    // Matrix access (2 columns)
    file.goToRec(1, 1, 2);
    ASSERT_EQ(file()->id, 8u);
    file.close();

    // Record too long
    RSmartTempFile tmp2;
    RRecFile<Person, true> bad(tmp2.path(), recSize);
    bad.open(RIO::Create);
    ASSERT_THROW(bad.writeRec(Person(10, std::string(50, 'x'))), RIOException);
}
