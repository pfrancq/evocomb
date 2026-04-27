/*

	EvoComb

	RBinaryFile.hpp

	Binary file - Header.

	Copyright 2006-2026 by Pascal Francq (pascal@francq.info).
	Copyright 2006-2008 by the Université Libre de Bruxelles (ULB).

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
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "io/r_io_file.hpp"


//------------------------------------------------------------------------------
namespace evocomb{
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
 * The RBinaryFile class represents a binary file in which values can be read and written with the >> and <<
 * operators:
 * - The arithmetic types (bool, char, int, std::size_t, double...) and the enumerations are written as their memory
 *   representation (sizeof(T) bytes).
 * - The strings are written as their length (a std::uint64_t) followed by their characters (without a final 0).
 *
 * The values are written in the byte order and with the sizes of the current platform: a file is not portable between
 * platforms with different endianness or sizes (for example, long has 4 bytes under Windows and 8 bytes under Linux
 * and macOS). Prefer types with a fixed size (std::int32_t, std::uint64_t...) for files that must be exchanged.
 *
 * Here is an example:
 * @code
 * evocomb::RBinaryFile file("/tmp/test.bin");
 * file.open(evocomb::RIO::Create);
 * file << 3.14 << std::uint32_t(42) << std::string("Hello");
 * file.close();
 *
 * double pi;
 * std::uint32_t nb;
 * std::string str;
 * file.open(evocomb::RIO::Read);
 * file >> pi >> nb >> str;
 * @endcode
 * @short Binary file.
 */
class RBinaryFile : public RIOFile
{
public:

    /**
     * Construct a binary file without path (see open).
     */
    RBinaryFile() = default;

    /**
     * Construct a binary file.
     * @param path               Path of the file.
     */
    explicit RBinaryFile(std::filesystem::path path) : RIOFile(std::move(path)) {}

    /**
     * Read a value (arithmetic type or enumeration).
     * @tparam T                 Type of the value.
     * @param value              Value read.
     * @exception RIOException   The value cannot be read (for example, the end of the file is reached).
     */
    template<class T, std::enable_if_t<std::is_arithmetic_v<T> || std::is_enum_v<T>, int> = 0>
        RBinaryFile& operator>>(T& value)
    {
        readBytes(&value, sizeof(T));
        return *this;
    }

    /**
     * Read a string (its length followed by its characters).
     * @param str                String read.
     * @exception RIOException   The string cannot be read.
     */
    RBinaryFile& operator>>(std::string& str);

    /**
     * Write a value (arithmetic type or enumeration).
     * @tparam T                 Type of the value.
     * @param value              Value to write.
     * @exception RIOException   The value cannot be written.
     */
    template<class T, std::enable_if_t<std::is_arithmetic_v<T> || std::is_enum_v<T>, int> = 0>
        RBinaryFile& operator<<(const T value)
    {
        write(reinterpret_cast<const char*>(&value), sizeof(T));
        return *this;
    }

    /**
     * Write a string (its length followed by its characters).
     * @param str                String to write.
     * @exception RIOException   The string cannot be written.
     */
    RBinaryFile& operator<<(std::string_view str);

    /**
     * Read exactly a given number of bytes.
     * @param buffer             Buffer (it must contain at least nb bytes).
     * @param nb                 Number of bytes to read.
     * @exception RIOException   There are less than nb bytes left in the file.
     */
    void readBytes(void* buffer, std::size_t nb);
};


}  //-------- End of namespace evocomb ---------------------------------------------
