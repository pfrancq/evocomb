/*

	EvoComb

	RConcat.hpp

	Concatenation of values into a string - Header.

	Copyright 2026 by Pascal Francq (pascal@francq.info).

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/



//------------------------------------------------------------------------------
#pragma once


//------------------------------------------------------------------------------
// include files for ANSI C/C++
#include <charconv>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <string>
#include <type_traits>


//------------------------------------------------------------------------------
namespace evocomb {
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
 * The RFixed structure represents a real number written with a fixed number of decimals in a string built by
 * RConcat::concat (and thus in the log). It should be created with RConcat::fixed:
 * @code
 * LOG_INFO(log, "Fi+=", evocomb::RConcat::fixed(sol->fiPlus(), 3));   // Fi+=0.125
 * @endcode
 */
struct RFixed
{
    double value_;                              ///< Value.
    int precision_;                             ///< Number of decimals.
};


//------------------------------------------------------------------------------
/**
 * Write a RFixed into a stream.
 * @param os                     Stream.
 * @param value                  Value.
 */
inline std::ostream& operator<<(std::ostream& os, const RFixed& value)
{
    const auto flags = os.flags();
    const auto precision = os.precision();
    os << std::fixed << std::setprecision(value.precision_) << value.value_;
    os.flags(flags);
    os.precision(precision);
    return os;
}


//------------------------------------------------------------------------------
/**
 * The RConcat class provides static methods to build strings by concatenating values. It is used by the LOG_* and
 * mThrow* macros.
 * @code
 * const auto msg = evocomb::RConcat::concat("Value ", 0.1, " at ", 3);        // "Value 0.1 at 3"
 * const auto fi = evocomb::RConcat::concat(evocomb::RConcat::fixed(1.0 / 3, 2));  // "0.33"
 * @endcode
 * @short Concatenation of values.
 */
class RConcat
{
public:

    /**
     * Create a real number to be written with a fixed number of decimals.
     * @param value              Value.
     * @param precision          Number of decimals.
     */
    [[nodiscard]] static RFixed fixed(const double value, const int precision)
    {
        return RFixed{value, precision};
    }

    /**
     * Concatenate values into a string. Real numbers are written with the shortest representation that gives back
     * exactly the same value (0.1 is written "0.1"). Booleans are written as "true" or "false". The other values are
     * written with their operator<<.
     * @param args               Values to concatenate.
     */
    template<typename... Args>
        [[nodiscard]] static std::string concat(const Args&... args)
    {
        std::ostringstream str;
        str << std::boolalpha;
        (append(str, args), ...);
        return str.str();
    }

    //-----------------------------------------------------
    RConcat() = delete;

private:

    /**
     * Append a value to a stream (see concat).
     * @param str                Stream.
     * @param value              Value.
     */
    template<typename T>
        static void append(std::ostringstream& str, const T& value)
    {
        if constexpr (std::is_floating_point_v<T>)
        {
            char buffer[64];
            const auto [ptr, ec] = std::to_chars(buffer, buffer + sizeof(buffer), value);
            if (ec == std::errc{})
                str.write(buffer, ptr - buffer);
            else
                str << value;
        }
        else
            str << value;
    }
};


}  //------- End of namespace evocomb ----------------------------------------------
