/*

    EvoComb

    RGoogleTest.hpp

    Base class for all the tests - Header.

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
#pragma once


//------------------------------------------------------------------------------
// include files for ANSI C/C++
#include <string>


//------------------------------------------------------------------------------
// include files for Google Test
#include "gtest/gtest.h"


//------------------------------------------------------------------------------
// include files for EvoComb
#include "app/r_logger.hpp"
#include "config.h"


//------------------------------------------------------------------------------
/**
 * The RGoogleTest class is the base class for all the tests. It provides a logger writing into the file "app.log" in
 * the main directory of the project.
 *
 * Each test file defines its own class deriving from RGoogleTest:
 * @code
 * LOG_MODULE("r.unit_tests");
 *
 * class RIOBasicTests : public RGoogleTest {};
 *
 * TEST_F(RIOBasicTests, text)
 * {
 *     LOG_INFO(logger, "Parsing test.json");
 *     ...
 * }
 * @endcode
 */
class RGoogleTest : public testing::Test
{
protected:
    /**
     * Logger shared by all the tests.
     */
    evocomb::RLogger& logger;

    RGoogleTest()
        : logger(getLogger())
    {
    }

    /**
     * Log the beginning of each test.
     */
    void SetUp() override
    {
        // The location is the one of the test
        const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
        logger.write(evocomb::RLogLevel::Info, "r.unit_tests", info ? info->file() : __FILE__, info ? info->line() : __LINE__, "Start " + testName());
    }

    /**
     * Log the end of each test.
     */
    void TearDown() override
    {
        const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
        const bool failed = info && info->result()->Failed();
        logger.write(failed ? evocomb::RLogLevel::Error : evocomb::RLogLevel::Info, "r.unit_tests", info ? info->file() : __FILE__, info ? info->line() : __LINE__, (failed ? "Failed " : "End ") + testName());
    }

private:

    /**
     * Get the logger. It is created at the first call and shared by all the tests (the log file is opened only once).
     */
    static evocomb::RLogger& getLogger()
    {
        static evocomb::RLogger log(R"({"path": ")" + PROJECT_ROOT + R"(app.log", "level": "debug"})");
        return log;
    }

    /**
     * Get the full name of the current test ("Suite.Test").
     */
    static std::string testName()
    {
        const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
        return info ? std::string(info->test_suite_name()) + "." + info->name() : std::string("?");
    }
};
