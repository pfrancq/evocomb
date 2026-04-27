/*

    EvoComb

    RLoggerTests.cpp

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
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "r_google_test.hpp"
#include "tools/r_exception.hpp"
using namespace evocomb;


//------------------------------------------------------------------------------
mLogModule("r.unit_tests");


//------------------------------------------------------------------------------
class RLoggerTests : public RGoogleTest
{
protected:

    /**
     * Get the path of a temporary log file.
     */
    static std::string tmpPath()
    {
        return (std::filesystem::temp_directory_path() / "evocomb_logger_test.log").string();
    }

    /**
     * Read the content of a file.
     */
    static std::string readFile(const std::string& path)
    {
        std::ifstream file(path);
        return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    }
};


//------------------------------------------------------------------------------
TEST_F(RLoggerTests, defaultScreen)
{
    const auto path = tmpPath();
    RLogger log(R"({"path": ")" + path + R"(", "level": "info"})");
    EXPECT_EQ(log.level(), RLogLevel::Info);
    EXPECT_EQ(log.screenLevel(), RLogLevel::Error);

    // By default, only the errors are displayed (on std::cerr)
    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();
    mLogError(log, "an error");
    mLogWarn(log, "a warning");
    mLogInfo(log, "an information");
    const auto out = testing::internal::GetCapturedStdout();
    const auto err = testing::internal::GetCapturedStderr();
    EXPECT_TRUE(out.empty());
    EXPECT_NE(err.find("an error"), std::string::npos);
    EXPECT_EQ(err.find("a warning"), std::string::npos);

    // Everything up to info is written in the file
    const auto content = readFile(path);
    EXPECT_NE(content.find("an error"), std::string::npos);
    EXPECT_NE(content.find("a warning"), std::string::npos);
    EXPECT_NE(content.find("an information"), std::string::npos);
    std::filesystem::remove(path);
}


//------------------------------------------------------------------------------
TEST_F(RLoggerTests, screen)
{
    const auto path = tmpPath();
    RLogger log(R"({"path": ")" + path + R"(", "level": "error",
        "screen": {"level": "info", "warning": "cout", "info": "cerr"}})");

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();
    mLogError(log, "an error");
    mLogWarn(log, "a warning");
    mLogInfo(log, "an information");
    const auto out = testing::internal::GetCapturedStdout();
    const auto err = testing::internal::GetCapturedStderr();
    EXPECT_NE(err.find("an error"), std::string::npos);
    EXPECT_NE(out.find("a warning"), std::string::npos);
    EXPECT_NE(err.find("an information"), std::string::npos);
    EXPECT_EQ(out.find("an error"), std::string::npos);

    // Only the errors are written in the file
    const auto content = readFile(path);
    EXPECT_NE(content.find("an error"), std::string::npos);
    EXPECT_EQ(content.find("a warning"), std::string::npos);

    // Nothing on the screen
    log.updateConfig(R"({"screen": {"level": "none"}})");
    EXPECT_EQ(log.level(), RLogLevel::Error);
    testing::internal::CaptureStderr();
    mLogError(log, "another error");
    EXPECT_TRUE(testing::internal::GetCapturedStderr().empty());
    std::filesystem::remove(path);
}


//------------------------------------------------------------------------------
TEST_F(RLoggerTests, config)
{
    const auto path = tmpPath();
    EXPECT_THROW(RLogger(R"({"level": "info"})"), RException);
    EXPECT_THROW(RLogger(R"({"path": ")" + path + R"("})"), RException);
    EXPECT_THROW(RLogger(R"({"path": ")" + path + R"(", "level": "verbose"})"), RException);
    EXPECT_THROW(RLogger(R"({"path": ")" + path + R"(", "level": "info", "screen": {"error": "stdout"}})"), RException);
    EXPECT_THROW(RLogger(R"({"path": ")" + path + R"(", "level": "info", "screen": "none"})"), RException);
    EXPECT_EQ(RLogger::toLevel("Warning"), RLogLevel::Warning);
    std::filesystem::remove(path);
}


//------------------------------------------------------------------------------
TEST_F(RLoggerTests, updateConfig)
{
    const auto path1 = tmpPath();
    const auto path2 = (std::filesystem::temp_directory_path() / "evocomb_logger_test2.log").string();
    RLogger log(R"({"path": ")" + path1 + R"(", "level": "info", "screen": {"level": "none"}})");
    mLogInfo(log, "first file");

    // Only the parameters present are modified
    log.updateConfig(R"({"level": "warning"})");
    EXPECT_EQ(log.level(), RLogLevel::Warning);
    EXPECT_EQ(log.screenLevel(), RLogLevel::None);
    EXPECT_EQ(log.path(), std::filesystem::path(path1));

    // Same path: the file is kept (not emptied)
    log.updateConfig(R"({"path": ")" + path1 + R"("})");
    mLogWarn(log, "still first file");
    EXPECT_NE(readFile(path1).find("first file"), std::string::npos);

    // New path: the current file is closed and the new one is opened
    log.updateConfig(R"({"path": ")" + path2 + R"("})");
    EXPECT_EQ(log.path(), std::filesystem::path(path2));
    mLogWarn(log, "second file");
    EXPECT_EQ(readFile(path1).find("second file"), std::string::npos);
    EXPECT_NE(readFile(path1).find("still first file"), std::string::npos);
    EXPECT_NE(readFile(path2).find("second file"), std::string::npos);

    // An invalid configuration modifies nothing
    EXPECT_THROW(log.updateConfig(R"({"path": "/nonexistent/dir/x.log", "level": "debug"})"), RException);
    EXPECT_THROW(log.updateConfig(R"({"level": "debug", "screen": {"info": "stdout"}})"), RException);
    EXPECT_EQ(log.level(), RLogLevel::Warning);
    EXPECT_EQ(log.path(), std::filesystem::path(path2));

    std::filesystem::remove(path1);
    std::filesystem::remove(path2);
}
