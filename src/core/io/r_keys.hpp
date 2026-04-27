/*

	EvoComb

	RKeys.hpp

	Keys for RKeyValueFile - Header.

	Copyright 2009-2026 by Pascal Francq (pascal@francq.info).

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
#include <cstring>
#include <string>


//------------------------------------------------------------------------------
namespace evocomb{
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
 * The RIntKey class represents a key of a RKeyValueFile made of one integer.
 * @short Integer Key.
 */
class RIntKey
{
public:

    /**
     * Size of a key in the file (in bytes).
     */
    static constexpr std::size_t size = sizeof(std::uint64_t);

    /**
     * Construct a key.
     * @param id                 Integer.
     */
    explicit RIntKey(const std::uint64_t id = 0) : id_(id) {}

    /**
     * Get the integer.
     */
    [[nodiscard]] std::uint64_t id() const { return id_; }

    /**
     * Write the key into a buffer of size bytes.
     * @param data               Buffer.
     */
    void toBytes(char* data) const { std::memcpy(data, &id_, size); }

    /**
     * Compare the key with a key stored in a buffer.
     * @param data               Buffer.
     * @return a negative value, 0 or a positive value if the key is respectively before, equal or after the stored key.
     */
    [[nodiscard]] int compare(const char* data) const
    {
        std::uint64_t stored;
        std::memcpy(&stored, data, size);
        return (id_ < stored) ? -1 : (id_ > stored ? 1 : 0);
    }

    /**
     * Get a string representing the key (for the error messages).
     */
    [[nodiscard]] std::string toString() const { return "(" + std::to_string(id_) + ")"; }

private:

    //-----------------------------------------------------
    std::uint64_t id_;                      ///< Integer.
};


//------------------------------------------------------------------------------
/**
 * The RIntsKey class represents a key of a RKeyValueFile made of two integers (ordered by the first one, then by the
 * second one).
 * @short Two Integers Key.
 */
class RIntsKey
{
public:

    /**
     * Size of a key in the file (in bytes).
     */
    static constexpr std::size_t size = 2 * sizeof(std::uint64_t);

    /**
     * Construct a key.
     * @param id1                First integer.
     * @param id2                Second integer.
     */
    explicit RIntsKey(const std::uint64_t id1 = 0, const std::uint64_t id2 = 0) : id1_(id1), id2_(id2) {}

    /**
     * Get the first integer.
     */
    [[nodiscard]] std::uint64_t id1() const { return id1_; }

    /**
     * Get the second integer.
     */
    [[nodiscard]] std::uint64_t id2() const { return id2_; }

    /**
     * Write the key into a buffer of size bytes.
     * @param data               Buffer.
     */
    void toBytes(char* data) const
    {
        std::memcpy(data, &id1_, sizeof(std::uint64_t));
        std::memcpy(data + sizeof(std::uint64_t), &id2_, sizeof(std::uint64_t));
    }

    /**
     * Compare the key with a key stored in a buffer.
     * @param data               Buffer.
     * @return a negative value, 0 or a positive value if the key is respectively before, equal or after the stored key.
     */
    [[nodiscard]] int compare(const char* data) const
    {
        std::uint64_t stored1, stored2;
        std::memcpy(&stored1, data, sizeof(std::uint64_t));
        std::memcpy(&stored2, data + sizeof(std::uint64_t), sizeof(std::uint64_t));
        if (id1_ != stored1)
            return id1_ < stored1 ? -1 : 1;
        return id2_ < stored2 ? -1 : (id2_ > stored2 ? 1 : 0);
    }

    /**
     * Get a string representing the key (for the error messages).
     */
    [[nodiscard]] std::string toString() const
    {
        return "(" + std::to_string(id1_) + "," + std::to_string(id2_) + ")";
    }

private:

    //-----------------------------------------------------
    std::uint64_t id1_;                     ///< First integer.
    std::uint64_t id2_;                     ///< Second integer.
};


}  //-------- End of namespace evocomb ---------------------------------------------
