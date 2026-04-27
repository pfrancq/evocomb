/*

    EvoComb

    IOBasicTest.cpp

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
#include "io/r_text_parser.hpp"
#include "io/r_json_parser.hpp"
#include "tools/r_json.hpp"
using namespace evocomb;


//------------------------------------------------------------------------------
mLogModule("r.unit_tests");


//------------------------------------------------------------------------------
class RIOBasicTests : public RGoogleTest
{
};


//------------------------------------------------------------------------------
TEST_F(RIOBasicTests, text)
{
    RTextParser text;
    text.parse(PROJECT_ROOT + "data/test/test.json");
    ASSERT_TRUE(text.readLine() == "{");
    ASSERT_TRUE(text.readLine() == R"("first_name": "John",)");
}


//------------------------------------------------------------------------------
TEST_F(RIOBasicTests, json)
{
    RJSONParser jsonFile;
    jsonFile.parse(PROJECT_ROOT + "data/test/test.json");
    RJSON json;
    jsonFile.populate(&json);
    jsonFile.parse(PROJECT_ROOT + "data/test/test.json");
    ASSERT_TRUE(json.dump() == R"({"first_name":"John","last_name":"Smith","is_alive":true,"age":27,"address":{"street_address":"21 2nd Street","city":"New York","state":"NY","postal_code":"10021-3100"},"phone_numbers":[{"type":"home","number":"212 555-1234"},{"type":"office","number":"646 555-4567"}],"children":["Catherine","Thomas","Trevor"],"spouse":null})");
}


//------------------------------------------------------------------------------
TEST_F(RIOBasicTests, populate_json)
{
    RJSONParser jsonFile;
    RJSON json;
    jsonFile.populate(&json);
    jsonFile.parse(PROJECT_ROOT + "data/test/test.json");
    ASSERT_TRUE(json.dump() == R"({"first_name":"John","last_name":"Smith","is_alive":true,"age":27,"address":{"street_address":"21 2nd Street","city":"New York","state":"NY","postal_code":"10021-3100"},"phone_numbers":[{"type":"home","number":"212 555-1234"},{"type":"office","number":"646 555-4567"}],"children":["Catherine","Thomas","Trevor"],"spouse":null})");
}