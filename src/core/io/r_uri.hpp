/*

	EvoComb

	RURI.hpp

	Uniform Resource Identifier - Header.

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
#pragma once


//------------------------------------------------------------------------------
// include files for ANSI C/C++
#include <cstddef>
#include <filesystem>
#include <ostream>
#include <string>
#include <string_view>


//------------------------------------------------------------------------------
namespace evocomb{
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
 * The RURI class represents a Uniform Resource Identifier (RFC 3986) and gives access to its parts:
 * @code
 *   https://www.example.org:8080/doc/index.html?lang=fr#top
 *   \___/   \_____________/ \__/\_____________/ \_____/ \_/
 *   scheme     authority    port     path       query  fragment
 * @endcode
 * A local path ("/home/user/file.txt", "~/file.txt", "C:\\Data\\file.txt" or "\\Data\\file.txt") is converted
 * into a URI with the "file" scheme ("file:/home/user/file.txt", "file:///C:/Data/file.txt"...), the backslashes
 * being replaced by slashes. For a local file, the path of the URI can be converted into a std::filesystem::path (see
 * toPath).
 *
 * A URI without "//" after the scheme (such as "mailto:user@example.org") has no authority: the rest is the path
 * (followed by an optional query and fragment).
 *
 * The parts are returned as std::string_view on the URI: they are only valid as long as the RURI is not modified or
 * destroyed.
 * @short Uniform Resource Identifier.
 */
class RURI
{
public:

    /**
     * Construct an empty URI.
     */
    RURI() = default;

    /**
     * Construct a URI from a string.
     * @param uri                URI or local path.
     * @exception RException     The string is not a valid URI.
     */
    explicit RURI(std::string uri);

    /**
     * Construct a URI from a string.
     * @param uri                URI or local path.
     * @exception RException     The string is not a valid URI.
     */
    explicit RURI(const char* uri) : RURI(std::string(uri ? uri : "")) {}

    /**
     * Construct a URI from a local path.
     * @param path               Path.
     */
    explicit RURI(const std::filesystem::path& path) : RURI(path.generic_string()) {}

    /**
     * Get the whole URI.
     */
    [[nodiscard]] const std::string& string() const { return uri_; }

    /**
     * Look if the URI is empty.
     */
    [[nodiscard]] bool empty() const { return uri_.empty(); }

    /**
     * Get the scheme (for example "https" or "file").
     */
    [[nodiscard]] std::string_view scheme() const { return extract(scheme_); }

    /**
     * Get the authority, i.e. the host with an optional user (for example "www.example.org").
     */
    [[nodiscard]] std::string_view authority() const { return extract(authority_); }

    /**
     * Get the port (for example "8080"), or an empty string if no port is specified.
     */
    [[nodiscard]] std::string_view port() const { return extract(port_); }

    /**
     * Get the path (for example "/doc/index.html").
     */
    [[nodiscard]] std::string_view path() const { return extract(path_); }

    /**
     * Get the query, without the "?" (for example "lang=fr").
     */
    [[nodiscard]] std::string_view query() const { return extract(query_); }

    /**
     * Get the fragment, without the "#" (for example "top").
     */
    [[nodiscard]] std::string_view fragment() const { return extract(fragment_); }

    /**
     * Get the extension of the last element of the path, without the "." (for example "html"), or an empty string if
     * there is no extension.
     */
    [[nodiscard]] std::string_view extension() const;

    /**
     * Look if the URI represents a local file (its scheme is "file").
     */
    [[nodiscard]] bool isLocal() const { return scheme() == "file"; }

    /**
     * Get the path of a local file.
     * @exception RException     The URI is not a local file.
     */
    [[nodiscard]] std::filesystem::path toPath() const;

    /**
     * Compare two URIs (the whole strings are compared).
     * @param uri                URI.
     * @return a negative value, 0 or a positive value if the URI is respectively before, equal or after uri.
     */
    [[nodiscard]] int compare(const RURI& uri) const { return uri_.compare(uri.uri_); }

    /**
     * Compare two URIs.
     * @param uri                URI.
     */
    [[nodiscard]] bool operator==(const RURI& uri) const { return uri_ == uri.uri_; }

    /**
     * Compare two URIs.
     * @param uri                URI.
     */
    [[nodiscard]] bool operator!=(const RURI& uri) const { return uri_ != uri.uri_; }

    /**
     * Compare two URIs (to use them in ordered containers).
     * @param uri                URI.
     */
    [[nodiscard]] bool operator<(const RURI& uri) const { return uri_ < uri.uri_; }

private:

    /**
     * The Part structure represents a part of the URI.
     */
    struct Part
    {
        std::size_t pos = 0;                ///< Position in the URI.
        std::size_t size = 0;               ///< Size of the part.
    };

    /**
     * Analyze the URI to find its parts.
     */
    void analyze();

    /**
     * Extract a part of the URI.
     * @param part               Part.
     */
    [[nodiscard]] std::string_view extract(const Part& part) const
    {
        return std::string_view(uri_).substr(part.pos, part.size);
    }

    //-----------------------------------------------------
    std::string uri_;                       ///< URI.
    Part scheme_;                           ///< Scheme.
    Part authority_;                        ///< Authority.
    Part port_;                             ///< Port.
    Part path_;                             ///< Path.
    Part query_;                            ///< Query.
    Part fragment_;                         ///< Fragment.
};


//------------------------------------------------------------------------------
/**
 * Write a URI into a stream.
 * @param os                     Stream.
 * @param uri                    URI.
 */
inline std::ostream& operator<<(std::ostream& os, const RURI& uri)
{
    return os << uri.string();
}


}  //-------- End of namespace evocomb ---------------------------------------------
