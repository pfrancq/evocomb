/*

    EvoComb

    IOCSVFileTests.cpp

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
#include <fstream>
#include <string>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "r_google_test.hpp"
#include "io/r_io.hpp"
#include "io/r_csv_file.hpp"
using namespace evocomb;


//------------------------------------------------------------------------------
mLogModule("r.unit_tests");


//------------------------------------------------------------------------------
class RCSVFileTests : public RGoogleTest
{
};


//------------------------------------------------------------------------------
TEST_F(RCSVFileTests, values)
{
    RCSVFile csv;
    csv.parse("\xEF\xBB\xBFname, value ,  \n"
              "\n"
              "   \r\n"
              " \"Smith, John\" , 3.5, \"He said \"\"hi\"\"\"\r\n"
              "\"multi\nline\",-2,\n"
              "a\\,b,\"  kept  \"");

    ASSERT_FALSE(csv.end());
    csv.read();
    ASSERT_EQ(csv.line(), 1u);
    ASSERT_EQ(csv.nbValues(), 3u);
    ASSERT_EQ(csv.value(0), "name");
    ASSERT_EQ(csv.value(1), "value");
    ASSERT_EQ(csv.value(2), "");

    csv.read();
    ASSERT_EQ(csv.line(), 4u);
    ASSERT_EQ(csv.value(0), "Smith, John");
    ASSERT_EQ(csv.value<double>(1), 3.5);
    ASSERT_EQ(csv.value(2), "He said \"hi\"");

    csv.read();
    ASSERT_EQ(csv.line(), 5u);
    ASSERT_EQ(csv.value(0), "multi\nline");
    ASSERT_EQ(csv.value<int>(1), -2);
    ASSERT_THROW(static_cast<void>(csv.value<unsigned>(1)), RIOException);
    ASSERT_EQ(csv.value<std::size_t>(2), 0u);
    ASSERT_THROW(static_cast<void>(csv.value<std::size_t>(2, false)), RIOException);
    ASSERT_THROW(static_cast<void>(csv.value(3)), RIOException);

    csv.read();
    ASSERT_EQ(csv.line(), 7u);
    ASSERT_EQ(csv.nbValues(), 2u);
    ASSERT_EQ(csv.value(0), "a,b");
    ASSERT_EQ(csv.value(1), "  kept  ");
    ASSERT_TRUE(csv.end());
    ASSERT_THROW(csv.read(), RIOException);
}


//------------------------------------------------------------------------------
TEST_F(RCSVFileTests, errorsAndOptions)
{
    RCSVFile csv;
    csv.parse("\"not closed,1\n");
    ASSERT_THROW(csv.read(), RIOException);
    csv.parse("\"a\" b,1\n");
    ASSERT_THROW(csv.read(), RIOException);
    ASSERT_THROW(static_cast<void>(csv.value<double>(0)), RIOException);

    // Tabulation as separator, no quotes and no escape character
    RCSVFile tsv('\t', false);
    tsv.escape(0);
    tsv.parse("\"a\"\t C:\\dir \t\t3\n");
    tsv.read();
    ASSERT_EQ(tsv.nbValues(), 4u);
    ASSERT_EQ(tsv.value(0), "\"a\"");
    ASSERT_EQ(tsv.value(1), "C:\\dir");
    ASSERT_EQ(tsv.value(2), "");
    ASSERT_EQ(tsv.value<double>(3), 3.0);

    ASSERT_EQ(RCSVFile::csvValue("say \"hi\""), "\"say \"\"hi\"\"\"");
}


//------------------------------------------------------------------------------
TEST_F(RCSVFileTests, file)
{
    RSmartTempFile tmp;
    std::ofstream(tmp.path()) << "id;name\n1;" << RCSVFile::csvValue("A; \"B\"") << "\n";
    RCSVFile csv(tmp.path(), ';');
    csv.open();
    csv.read();
    csv.read();
    ASSERT_EQ(csv.value<std::uint32_t>(0), 1u);
    ASSERT_EQ(csv.value(1), "A; \"B\"");
    ASSERT_TRUE(csv.end());

    RCSVFile missing(tmp.path().string() + ".missing");
    ASSERT_THROW(missing.open(), RIOException);
}
