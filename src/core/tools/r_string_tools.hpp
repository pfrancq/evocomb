/*

    EvoComb

    RStringTools.hpp

    String Tools - Header.

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
#include <algorithm>
#include <cctype>
#include <string>


//------------------------------------------------------------------------------
namespace evocomb{
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
 * The RStringTools class provides static methods to manipulate strings.
 * @short String tools.
 */
class RStringTools
{
public:

    /**
     * In place trim of a string from start.
     * @param s                  String to trim.
     */
    static void ltrim(std::string& s)
    {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](const unsigned char ch) {
            return !std::isspace(ch);
        }));
    }

    /**
     * In place trim of a string from end.
     * @param s                  String to trim.
     */
    static void rtrim(std::string& s)
    {
        s.erase(std::find_if(s.rbegin(), s.rend(), [](const unsigned char ch) {
            return !std::isspace(ch);
        }).base(), s.end());
    }

    /**
     * In place trim of a string from both ends.
     * @param s                  String to trim.
     */
    static void trim(std::string& s)
    {
        rtrim(s);
        ltrim(s);
    }

    //-----------------------------------------------------
    RStringTools() = delete;
};


}  //-------- End of namespace evocomb ---------------------------------------------
