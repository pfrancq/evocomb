/*

    EvoComb

    URITests.cpp

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
// include files for EvoComb
#include "r_google_test.hpp"
#include "io/r_uri.hpp"
#include "tools/r_exception.hpp"
using namespace evocomb;


//------------------------------------------------------------------------------
mLogModule("r.unit_tests");


//------------------------------------------------------------------------------
class RURITests : public RGoogleTest
{
};


//------------------------------------------------------------------------------
TEST_F(RURITests, url)
{
    const RURI uri("https://www.example.org:8080/doc/index.html?lang=fr#top");
    ASSERT_EQ(uri.scheme(), "https");
    ASSERT_EQ(uri.authority(), "www.example.org");
    ASSERT_EQ(uri.port(), "8080");
    ASSERT_EQ(uri.path(), "/doc/index.html");
    ASSERT_EQ(uri.query(), "lang=fr");
    ASSERT_EQ(uri.fragment(), "top");
    ASSERT_EQ(uri.extension(), "html");
    ASSERT_FALSE(uri.isLocal());
    ASSERT_THROW(static_cast<void>(uri.toPath()), RException);

    const RURI home("http://user:pwd@example.org");
    ASSERT_EQ(home.authority(), "user:pwd@example.org");
    ASSERT_EQ(home.port(), "");
    ASSERT_EQ(home.path(), "");

    const RURI query("http://example.org?q=1");
    ASSERT_EQ(query.path(), "");
    ASSERT_EQ(query.query(), "q=1");

    const RURI mail("mailto:user@example.org");
    ASSERT_EQ(mail.scheme(), "mailto");
    ASSERT_EQ(mail.authority(), "");
    ASSERT_EQ(mail.path(), "user@example.org");
}


//------------------------------------------------------------------------------
TEST_F(RURITests, local)
{
    const RURI posix("/home/user/data.tar.gz");
    ASSERT_EQ(posix.string(), "file:/home/user/data.tar.gz");
    ASSERT_TRUE(posix.isLocal());
    ASSERT_EQ(posix.path(), "/home/user/data.tar.gz");
    ASSERT_EQ(posix.extension(), "gz");
    ASSERT_EQ(posix.toPath(), std::filesystem::path("/home/user/data.tar.gz"));

    const RURI windows(R"(C:\Data\file.txt)");
    ASSERT_EQ(windows.string(), "file:///C:/Data/file.txt");
    ASSERT_EQ(windows.path(), "C:/Data/file.txt");

    const RURI file("file:///tmp/test.json");
    ASSERT_EQ(file.path(), "/tmp/test.json");
    ASSERT_EQ(RURI(std::filesystem::path("/tmp/test.json")).path(), "/tmp/test.json");

    ASSERT_EQ(RURI("/dir.d/file").extension(), "");
    ASSERT_EQ(RURI("/home/.bashrc").extension(), "");
}


//------------------------------------------------------------------------------
TEST_F(RURITests, invalid)
{
    ASSERT_THROW(RURI("no scheme"), RException);
    ASSERT_THROW(RURI("1http://example.org"), RException);
    ASSERT_TRUE(RURI().empty());
    ASSERT_TRUE(RURI("http://a.org") == RURI("http://a.org"));
    ASSERT_TRUE(RURI("http://a.org") < RURI("http://b.org"));
}
