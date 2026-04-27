/*

	EvoComb

	RURI.cpp

	Uniform Resource Identifier - Implementation.

	Copyright 1999-2026 by Pascal Francq (pascal@francq.info).
	Copyright 1999-2008 by the Université Libre de Bruxelles (ULB).

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
#include <cctype>
#include <utility>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "io/r_uri.hpp"
#include "tools/r_exception.hpp"
#include "app/r_logger.hpp"
using namespace evocomb;


//------------------------------------------------------------------------------
mLogModule("r.core");


//------------------------------------------------------------------------------
/**
 * Look if a character can be used in a scheme (after the first letter).
 * @param c                      Character.
 */
static bool isSchemeChar(const char c)
{
    return std::isalnum(static_cast<unsigned char>(c)) || c == '+' || c == '-' || c == '.';
}



//------------------------------------------------------------------------------
//
// class RURI
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
RURI::RURI(std::string uri) :
    uri_(std::move(uri))
{
    analyze();
}


//------------------------------------------------------------------------------
void RURI::analyze()
{
    scheme_ = authority_ = port_ = path_ = query_ = fragment_ = Part{};
    if (uri_.empty())
        return;

    // Local path: Unix ("/..." or "~..."), Windows with a drive ("C:\...") or without ("\...")
    const bool drive = uri_.size() >= 2 && std::isalpha(static_cast<unsigned char>(uri_[0])) && uri_[1] == ':' &&
                       (uri_.size() == 2 || uri_[2] == '\\' || uri_[2] == '/');
    if (uri_[0] == '/' || uri_[0] == '~' || uri_[0] == '\\' || drive)
    {
        std::replace(uri_.begin(), uri_.end(), '\\', '/');
        uri_.insert(0, drive ? "file:///" : "file:");
        scheme_ = {0, 4};
        path_.pos = drive ? 8 : 5;
        path_.size = uri_.size() - path_.pos;
        return;
    }

    // Scheme: a letter followed by letters, digits, "+", "-" or "." until ":"
    std::size_t i = 0;
    if (!std::isalpha(static_cast<unsigned char>(uri_[0])))
        mThrowRException("'", uri_, "' is not a valid URI");
    while (i < uri_.size() && isSchemeChar(uri_[i]))
        ++i;
    if (i == uri_.size() || uri_[i] != ':')
        mThrowRException("'", uri_, "' is not a valid URI");
    scheme_ = {0, i};
    ++i;

    // Local file: the rest is the path (a "//" is skipped, and "file://host/path" is not supported)
    if (scheme() == "file")
    {
        if (uri_.compare(i, 2, "//") == 0)
            i += 2;
        path_ = {i, uri_.size() - i};
        return;
    }

    // Authority and port ("//authority:port")
    if (uri_.compare(i, 2, "//") == 0)
    {
        i += 2;
        const auto end = std::min(uri_.find_first_of("/?#", i), uri_.size());
        auto colon = uri_.find(':', i);
        if (const auto at = uri_.find('@', i); at < end && colon < at)
            colon = uri_.find(':', at);     // A ":" in the user information is not a port
        if (colon < end)
        {
            authority_ = {i, colon - i};
            port_ = {colon + 1, end - colon - 1};
        }
        else
            authority_ = {i, end - i};
        i = end;
    }

    // Path, query and fragment
    const auto endPath = std::min(uri_.find_first_of("?#", i), uri_.size());
    path_ = {i, endPath - i};
    i = endPath;
    if (i < uri_.size() && uri_[i] == '?')
    {
        ++i;
        const auto endQuery = std::min(uri_.find('#', i), uri_.size());
        query_ = {i, endQuery - i};
        i = endQuery;
    }
    if (i < uri_.size() && uri_[i] == '#')
    {
        ++i;
        fragment_ = {i, uri_.size() - i};
    }
}


//------------------------------------------------------------------------------
std::string_view RURI::extension() const
{
    auto name = path();
    if (const auto slash = name.rfind('/'); slash != std::string_view::npos)
        name.remove_prefix(slash + 1);
    const auto dot = name.rfind('.');
    if (dot == std::string_view::npos || dot == 0)
        return {};
    return name.substr(dot + 1);
}


//------------------------------------------------------------------------------
std::filesystem::path RURI::toPath() const
{
    if (!isLocal())
        mThrowRException("'", uri_, "' is not a local file");
    return std::filesystem::path(std::string(path()));
}
