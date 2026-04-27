/*

    EvoComb

    ToolsJSONTests.cpp

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
// include files for ANSI C/C++
#include <memory>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "r_google_test.hpp"
#include "tools/r_json.hpp"
using namespace evocomb;


//------------------------------------------------------------------------------
mLogModule("r.unit_tests");


//------------------------------------------------------------------------------
class RToolsJSONTests : public RGoogleTest
{
};


//------------------------------------------------------------------------------
TEST_F(RToolsJSONTests, build)
{
    RJSON root;
    root.add("firstname", std::make_unique<RJSON>("pascal"));
    root.add("lastname", std::make_unique<RJSON>("francq"));
    const auto address = root.add("address", std::make_unique<RJSON>(RJSONType::Object));
    address->add("Street",std::make_unique<RJSON>("146 Chaussée de Boitsfort"));
    address->add("Zip",std::make_unique<RJSON>(1170));
    address->add("City", std::make_unique<RJSON>("Watermael-Boitsfort"));
    const auto albums = root.add("albums", std::make_unique<RJSON>(RJSONType::Array));
    albums->add(std::make_unique<RJSON>( "Rêves électroniques"));
    albums->add(std::make_unique<RJSON>("Phasmes"));
    ASSERT_TRUE(root.dump() == R"({"firstname":"pascal","lastname":"francq","address":{"Street":"146 Chaussée de Boitsfort","Zip":1170,"City":"Watermael-Boitsfort"},"albums":["Rêves électroniques","Phasmes"]})");
}


//------------------------------------------------------------------------------
TEST_F(RToolsJSONTests, replace)
{
    RJSON root;
    root.add("firstname", std::make_unique<RJSON>("pascal"));
    const auto item = root.add("firstname", std::make_unique<RJSON>(RJSONType::Object));
    item->add("firstname", std::make_unique<RJSON>("pascal"));
    item->add("suffix", std::make_unique<RJSON>("Jr."));
    std::cout << root.dump() << std::endl;
    ASSERT_TRUE(root.dump() == R"({"firstname":{"firstname":"pascal","suffix":"Jr."}})");
}

//------------------------------------------------------------------------------
TEST_F(RToolsJSONTests, copyAndDumpChild)
{
    const auto json = RJSON::parse(R"({"a": {"b": 1, "c": [1, 2]}, "d": true})");

    // An object dumped alone does not contain its key
    const auto a = json.find("a");
    ASSERT_NE(a, nullptr);
    EXPECT_EQ(a->dump(), R"({"b":1,"c":[1,2]})");
    EXPECT_EQ(RJSON::parse(a->dump()).dump(), a->dump());

    // Copy of an object
    const RJSON copy(json);
    EXPECT_EQ(copy.dump(), json.dump());
    EXPECT_EQ(copy["a"]["b"].toInteger(), 1);
}


//------------------------------------------------------------------------------
TEST_F(RToolsJSONTests, escape)
{
    // Special characters are escaped when dumped and decoded when parsed
    RJSON root;
    root.add("say \"hi\"", std::make_unique<RJSON>("a \"quote\", a \\ and\na new line"));
    const auto dump = root.dump();
    EXPECT_EQ(dump, R"({"say \"hi\"":"a \"quote\", a \\ and\na new line"})");
    const auto parsed = RJSON::parse(dump);
    EXPECT_EQ(parsed["say \"hi\""].toString(), "a \"quote\", a \\ and\na new line");

    // Unicode escape sequences are decoded in UTF-8
    const auto unicode = RJSON::parse(R"({"a": "Réseau € 😀 \/"})");
    EXPECT_EQ(unicode["a"].toString(), "Réseau € \xF0\x9F\x98\x80 /");
    EXPECT_THROW(static_cast<void>(RJSON::parse(R"({"a": "\x"})")), std::exception);
}


//------------------------------------------------------------------------------
TEST_F(RToolsJSONTests, numbers)
{
    // The decimal values keep their precision and remain decimal values
    RJSON root;
    root.add("small", std::make_unique<RJSON>(1.0e-7));
    root.add("third", std::make_unique<RJSON>(1.0 / 3.0));
    root.add("round", std::make_unique<RJSON>(2.0));
    const auto parsed = RJSON::parse(root.dump());
    EXPECT_TRUE(parsed["small"].isDecimal());
    EXPECT_DOUBLE_EQ(parsed["small"].toDecimal(), 1.0e-7);
    EXPECT_EQ(parsed["third"].toDecimal(), 1.0 / 3.0);
    EXPECT_TRUE(parsed["round"].isDecimal());
    EXPECT_EQ(parsed["round"].dump(), R"({"round":2.0})");

    // Size of the objects and the arrays
    const auto json = RJSON::parse(R"({"a": [1, 2, 3], "b": {"c": 1}, "d": 4})");
    EXPECT_EQ(json.size(), 3u);
    EXPECT_EQ(json["a"].size(), 3u);
    EXPECT_EQ(json["b"].size(), 1u);
    EXPECT_EQ(json["d"].size(), 0u);
}


//------------------------------------------------------------------------------
TEST_F(RToolsJSONTests, merge)
{
    auto config = RJSON::parse(R"({"ga": {"seed": 1, "nb_threads": 1}, "name": "a"})");
    config.merge(RJSON::parse(R"({"ga": {"nb_threads": 4}, "name": {"first": "b"}, "new": [1, 2]})"));
    EXPECT_EQ(config["ga"]["seed"].toInteger(), 1);
    EXPECT_EQ(config["ga"]["nb_threads"].toInteger(), 4);
    EXPECT_EQ(config["name"]["first"].toString(), "b");
    EXPECT_EQ(config["new"].size(), 2u);
    EXPECT_THROW(config.merge(RJSON::parse("[1]")), std::exception);

    // The elements of an object are accessible by index
    EXPECT_EQ(config["ga"][0].key(), "seed");
    EXPECT_EQ(config["ga"][1].toInteger(), 4);
}


//------------------------------------------------------------------------------
TEST_F(RToolsJSONTests, pretty)
{
    // Short elements are written on a single line
    const auto json = RJSON::parse(R"({"a": 1, "b": [], "c": {"d": [true, null]}, "e": {}})");
    EXPECT_EQ(json.dump(2), R"({"a": 1, "b": [], "c": {"d": [true, null]}, "e": {}})");

    // Long elements are indented (with their short children on a single line)
    std::string numbers;
    std::string expected = "{\n  \"long\": [\n";
    for (int i = 0; i < 30; ++i)
    {
        numbers += (i ? ", " : "") + std::to_string(1000 + i);
        expected += "    " + std::to_string(1000 + i) + (i < 29 ? ",\n" : "\n");
    }
    expected += "  ],\n  \"short\": {\"a\": [1, 2], \"b\": \"x\"}\n}";
    const auto big = RJSON::parse(R"({"long": [)" + numbers + R"(], "short": {"a": [1, 2], "b": "x"}})");
    EXPECT_EQ(big.dump(2), expected);
    EXPECT_EQ(RJSON::parse(big.dump(2)).dump(), big.dump());
}
