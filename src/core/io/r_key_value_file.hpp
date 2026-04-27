/*

	EvoComb

	RKeyValueFile.hpp

	Generic Key-Value File - Header.

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
#include <filesystem>
#include <type_traits>
#include <utility>
#include <vector>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "io/r_block_file.hpp"
#include "app/r_logger.hpp"


//------------------------------------------------------------------------------
namespace evocomb{
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
 * The RKeyValueFile class represents a file storing (key, value) pairs, such as an inverted file used by a search
 * engine. The approach is based on Zobel, Moffat and Sacks-Davis (1993):
 * - The file is made of blocks (see RBlockFile), and each pair is stored in a given block. The caller must remember
 *   the block of each key (for example in a dictionary), since the same key may exist in several blocks.
 * - All the keys have the same size, and a value cannot be larger than a block.
 *
 * A block is organized as follows:
 * -# The number of free bytes in the block (std::uint64_t).
 * -# The number of pairs in the block (std::uint64_t).
 * -# The address table: for each pair (ordered by key), the key followed by the position of the value in the block
 *    (std::uint64_t).
 * -# The free space.
 * -# The values: as a memory heap, they are stored from the end of the block (the value of the first pair of the
 *    table is at the end of the block).
 *
 * To write a value, seek(std::size_t&, const K&, std::size_t) creates or resizes the pair (in a new block if
 * necessary) and moves to the value, which is then written with write. To read a value, seek(std::size_t, const K&)
 * moves to the value, which is then read with read. A read or a write cannot cross the end of the value.
 *
 * The class of the keys must provide (see RIntKey and RIntsKey):
 * @code
 * static constexpr std::size_t size;          // Size of a key in the file (in bytes).
 * void toBytes(char* data) const;             // Write the key into a buffer of size bytes.
 * int compare(const char* data) const;        // Compare the key with a stored key (< 0, 0 or > 0).
 * std::string toString() const;               // String representing the key (for the error messages).
 * @endcode
 *
 * The class also provides methods to read and write a vector of numbers:
 * @code
 * evocomb::RKeyValueFile<evocomb::RIntKey> file("/tmp/test.bin", 10240, 2, 1024);   // Blocks of 10 KB, 2 in cache
 * file.open();
 * std::size_t block = 0;                     // No block is assigned to the key yet
 * file.write(block, evocomb::RIntKey(300), std::vector<std::uint32_t>{3, 6, 7, 10});
 * std::vector<std::uint32_t> values;
 * file.read(block, evocomb::RIntKey(300), values);
 * @endcode
 * @tparam K                     Class of the keys.
 * @short Generic Key-Value File.
 */
template<class K>
    class RKeyValueFile
{
public:

    /**
     * Construct a file.
     * @param path               Path of the file.
     * @param blockSize          Size of a block (in bytes).
     * @param nbCaches           Number of blocks kept in memory.
     * @param tolerance          Minimal number of free bytes that a block must have to receive a new pair (in bytes).
     */
    RKeyValueFile(std::filesystem::path path, const std::size_t blockSize, const std::size_t nbCaches, const std::size_t tolerance):
        blocks_(std::move(path), blockSize, nbCaches), tolerance_(tolerance) {}

    /**
     * Type of the cache (see RBlockFile::CacheType).
     */
    using CacheType = RBlockFile::CacheType;

    /**
     * Get the path of the file.
     */
    [[nodiscard]] const std::filesystem::path& path() const { return blocks_.path(); }

    /**
     * Look if the file is open.
     */
    [[nodiscard]] bool isOpen() const { return blocks_.isOpen(); }

    /**
     * Get the size of the blocks.
     */
    [[nodiscard]] std::size_t blockSize() const { return blocks_.blockSize(); }

    /**
     * Get the number of blocks.
     */
    [[nodiscard]] std::size_t nbBlocks() const { return blocks_.nbBlocks(); }

    /**
     * Get the type of the cache.
     */
    [[nodiscard]] CacheType cacheType() const { return blocks_.cacheType(); }

    /**
     * Set the type of the cache. If the type becomes CacheType::WriteThrough, the modified blocks are saved.
     * @param type               Type.
     */
    void cacheType(const CacheType type) { blocks_.cacheType(type); }

    /**
     * Save all the modified blocks of the cache on the disk.
     */
    void flush() { blocks_.flush(); }

    /**
     * Close the file (the modified blocks are saved).
     */
    void close()
    {
        blocks_.close();
        recordBlock_ = 0;
    }

    /**
     * Open the file (in read and write mode). It is created if necessary.
     * @exception RIOException   The file cannot be open.
     */
    void open()
    {
        blocks_.open();
        freeSpaces_.assign(nbBlocks(), 0);
        for (std::size_t id = 1; id <= nbBlocks(); ++id)
        {
            const char* data = blocks_.blockData(id);
            freeSpaces_[id - 1] = get(data, 0) == 0 && get(data, Len) == 0 ? blockSize() - Len2 : get(data, 0);
        }
        recordBlock_ = 0;
    }

    /**
     * Remove all the pairs from the file.
     */
    void clear()
    {
        blocks_.clear();
        freeSpaces_.clear();
        recordBlock_ = 0;
    }

    /**
     * Look if a pair exists in a given block.
     * @param blockId            Identifier of the block.
     * @param key                Key.
     */
    [[nodiscard]] bool exists(const std::size_t blockId, const K& key)
    {
        if (!blockId || blockId > nbBlocks())
            return false;
        bool found;
        findEntry(blocks_.blockData(blockId), key, found);
        return found;
    }

    /**
     * Go to the value of a pair to read it.
     * @param blockId            Identifier of the block.
     * @param key                Key.
     * @exception RIOException   The pair does not exist.
     */
    void seek(const std::size_t blockId, const K& key)
    {
        mLogModule("r.core");
        verifyBlock(blockId);
        const char* data = blocks_.blockData(blockId);
        bool found;
        const auto entry = findEntry(data, key, found);
        if (!found)
            mThrowRIOException(file(), "No record ", key.toString(), " in block ", blockId);
        position(blockId, data, entry);
    }

    /**
     * Go to the value of a pair to write it, the value having a given size:
     * - If the block is null, the pair is created in a block with enough free space (or in a new block), and the
     *   identifier of the block is updated.
     * - If the pair exists in the block, it is resized if necessary. If the block has not enough free space, the pair
     *   is moved to another block (its value is lost) and the identifier of the block is updated.
     * @param blockId            Identifier of the block (0 for a new pair).
     * @param key                Key.
     * @param size               Size of the value (in bytes).
     * @exception RIOException   The pair does not exist in the given block, or it cannot be stored in a block.
     */
    void seek(std::size_t& blockId, const K& key, const std::size_t size)
    {
        mLogModule("r.core");
        if (size + K::size + Len > blockSize() - Len2)
            mThrowRIOException(file(), "The size of a record (", size, ") exceeds the block size (", blockSize(), ")");

        // Existing pair
        if (blockId)
        {
            verifyBlock(blockId);
            char* data = blocks_.blockData(blockId);
            bool found;
            const auto entry = findEntry(data, key, found);
            if (!found)
                mThrowRIOException(file(), "No record ", key.toString(), " in block ", blockId);
            const auto oldSize = end(data, entry) - pos(data, entry);
            if (size <= oldSize || size - oldSize <= freeSpaces_[blockId - 1])
            {
                resize(blockId, data, entry, static_cast<std::ptrdiff_t>(size) - static_cast<std::ptrdiff_t>(oldSize));
                position(blockId, data, entry);
                return;
            }
            erase(blockId, key);
        }

        // Find a block with enough free space and not containing the key yet, or create a new one
        const auto needed = size + K::size + Len;
        for (blockId = 1; blockId <= nbBlocks(); ++blockId)
        {
            if (freeSpaces_[blockId - 1] <= tolerance_ || freeSpaces_[blockId - 1] < needed)
                continue;
            char* data = blocks_.blockData(blockId);
            bool found;
            const auto entry = findEntry(data, key, found);
            if (found)
                continue;
            insert(blockId, data, entry, key, size);
            position(blockId, data, entry);
            return;
        }
        blockId = nbBlocks() + 1;
        char* data = blocks_.blockData(blockId);
        freeSpaces_.push_back(blockSize() - Len2);
        insert(blockId, data, 0, key, size);
        position(blockId, data, 0);
    }

    /**
     * Remove a pair.
     * @param blockId            Identifier of the block.
     * @param key                Key.
     * @exception RIOException   The pair does not exist.
     */
    void erase(const std::size_t blockId, const K& key)
    {
        mLogModule("r.core");
        verifyBlock(blockId);
        char* data = blocks_.blockData(blockId);
        bool found;
        const auto entry = findEntry(data, key, found);
        if (!found)
            mThrowRIOException(file(), "No record ", key.toString(), " in block ", blockId);
        const auto nb = get(data, Len);
        const auto start = pos(data, entry);
        const auto size = end(data, entry) - start;
        const auto lowest = pos(data, nb - 1);

        // Move the values of the next pairs and update their positions, then remove the entry from the table
        std::memmove(data + lowest + size, data + lowest, start - lowest);
        for (auto i = entry + 1; i < nb; ++i)
            setPos(data, i, pos(data, i) + size);
        std::memmove(data + entryOffset(entry), data + entryOffset(entry + 1), (nb - entry - 1) * EntrySize);
        std::memset(data + entryOffset(nb - 1), 0, EntrySize);
        set(data, Len, nb - 1);
        setFree(blockId, data, freeSpaces_[blockId - 1] + size + EntrySize);
        blocks_.blockModified();
        recordBlock_ = 0;
    }

    /**
     * Get the size of the value of the current pair (see seek).
     */
    [[nodiscard]] std::size_t recordSize() const { return recordEnd_ - recordStart_; }

    /**
     * Read a given number of bytes of the current value.
     * @param buffer             Buffer (it must contain at least nb bytes).
     * @param nb                 Number of bytes to read.
     * @exception RIOException   There is no current value or the read crosses the end of the value.
     */
    void read(char* buffer, const std::size_t nb)
    {
        verifyRecord(nb);
        blocks_.read(buffer, nb);
    }

    /**
     * Write a given number of bytes in the current value.
     * @param buffer             Buffer.
     * @param nb                 Number of bytes to write.
     * @exception RIOException   There is no current value or the write crosses the end of the value.
     */
    void write(const char* buffer, const std::size_t nb)
    {
        verifyRecord(nb);
        blocks_.write(buffer, nb);
    }

    /**
     * Write a vector of numbers as the value of a pair (its size, a std::uint64_t, followed by the numbers).
     * @tparam T                 Type of the numbers.
     * @param blockId            Identifier of the block (see seek(std::size_t&, const K&, std::size_t)).
     * @param key                Key.
     * @param values             Numbers.
     */
    template<class T>
        void write(std::size_t& blockId, const K& key, const std::vector<T>& values)
    {
        static_assert(std::is_arithmetic_v<T>, "Only vectors of numbers are supported");
        const auto nb = static_cast<std::uint64_t>(values.size());
        seek(blockId, key, sizeof(nb) + values.size() * sizeof(T));
        write(reinterpret_cast<const char*>(&nb), sizeof(nb));
        write(reinterpret_cast<const char*>(values.data()), values.size() * sizeof(T));
    }

    /**
     * Read a vector of numbers written with write(std::size_t&, const K&, const std::vector<T>&).
     * @tparam T                 Type of the numbers.
     * @param blockId            Identifier of the block.
     * @param key                Key.
     * @param values             Numbers.
     */
    template<class T>
        void read(const std::size_t blockId, const K& key, std::vector<T>& values)
    {
        static_assert(std::is_arithmetic_v<T>, "Only vectors of numbers are supported");
        mLogModule("r.core");
        seek(blockId, key);
        std::uint64_t nb = 0;
        read(reinterpret_cast<char*>(&nb), sizeof(nb));
        if (nb * sizeof(T) != recordSize() - sizeof(nb))
            mThrowRIOException(file(), "The record ", key.toString(), " is not a vector of this type");
        values.resize(static_cast<std::size_t>(nb));
        read(reinterpret_cast<char*>(values.data()), values.size() * sizeof(T));
    }

private:

    /**
     * Read an integer in a block.
     * @param data                      Data representing the block.
     * @param offset                    Offset inside the block.
     */
    static std::uint64_t get(const char* data, const std::size_t offset)
    {
        std::uint64_t value;
        std::memcpy(&value, data + offset, Len);
        return value;
    }

    /**
     * Write a number in a block.
     * @param data                      Data representing the block.
     * @param offset                    Offset inside the block.
     * @param value                     Value to write.
     */
    static void set(char* data, const std::size_t offset, const std::uint64_t value) { std::memcpy(data + offset, &value, Len); }

    /**
     * Get the offset of an entry of the address table.
     * @param entry                     Entry index.
     */
    static std::size_t entryOffset(const std::size_t entry) { return Len2 + entry * EntrySize; }

    /**
     * Get the position of the value of an entry in a bloc.
     * @param data                      Data representing the bloc.
     * @param entry                     Entry index.
     */
    static std::size_t pos(const char* data, const std::size_t entry) { return get(data, entryOffset(entry) + K::size); }

    /**
     * Set the position of the value of an entry in a block.
     * @param data                      Data representing the bloc.
     * @param entry                     Entry index.
     * @param value                     Value.
     */
    static void setPos(char* data, const std::size_t entry, const std::size_t value) { set(data, entryOffset(entry) + K::size, value); }

    /**
     * Get the end of the value of an entry.
     */
    std::size_t end(const char* data, const std::size_t entry) const { return entry ? pos(data, entry - 1) : blockSize(); }

    /**
     * Find an entry with a binary search.
     * @param data               Data of the block.
     * @param key                Key.
     * @param found              Is the key found?
     * @return the index of the entry, or the index where it should be inserted.
     */
    static std::size_t findEntry(const char* data, const K& key, bool& found)
    {
        std::size_t min = 0;
        std::size_t max = get(data, Len);
        found = false;
        while (min < max)
        {
            const auto mid = min + (max - min) / 2;
            const auto comp = key.compare(data + entryOffset(mid));
            if (comp == 0)
            {
                found = true;
                return mid;
            }
            if (comp < 0)
                max = mid;
            else
                min = mid + 1;
        }
        return min;
    }

    /**
     * Get the underlying file (for the exceptions).
     */
    [[nodiscard]] const RIOFile* file() const { return &blocks_.file_; }

    /**
     * Verify that a block exists.
     */
    void verifyBlock(const std::size_t blockId) const
    {
        mLogModule("r.core");
        if (!isOpen())
            mThrowRIOException(file(), "File not opened");
        if (!blockId || blockId > nbBlocks())
            mThrowRIOException(file(), "There is no block ", blockId);
    }

    /**
     * Verify that a given number of bytes can be read or written in the current value.
     * @param nb                Number of bytes.
     */
    void verifyRecord(const std::size_t nb) const
    {
        mLogModule("r.core");
        if (!recordBlock_ || recordBlock_ != blocks_.currentBlock() || blocks_.currentPos() < recordStart_)
            mThrowRIOException(file(), "No current record");
        if (blocks_.currentPos() + nb > recordEnd_)
            mThrowRIOException(file(), "Cannot access ", nb, " bytes: the record has only ",
                               recordEnd_ - blocks_.currentPos(), " bytes left");
    }

    /**
     * Set the free space of a block.
     * @param blockId           Block identifier.
     * @param data              Data to fill.
     * @param free              Size.
     */
    void setFree(const std::size_t blockId, char* data, const std::size_t free)
    {
        freeSpaces_[blockId - 1] = free;
        set(data, 0, free);
    }

    /**
     * Go to the value of an entry.
     * @param blockId           Block identifier.
     * @param data              Data from which to extract the value.
     * @param entry
     */
    void position(const std::size_t blockId, const char* data, const std::size_t entry)
    {
        recordStart_ = pos(data, entry);
        recordEnd_ = end(data, entry);
        recordBlock_ = blockId;
        blocks_.seek(blockId, recordStart_);
    }

    /**
     * Insert a new entry whose value has a given size (the block must have enough free space).
     * @param blockId           Block identifier.
     * @param data              Data representing the block.
     * @param entry             Entry index.
     * @param key               Key.
     * @param size              Size of the entry.
     */
    void insert(const std::size_t blockId, char* data, const std::size_t entry, const K& key, const std::size_t size)
    {
        const auto nb = get(data, Len);
        const auto last = end(data, entry);
        const auto lowest = nb ? pos(data, nb - 1) : blockSize();

        // Move the values of the next entries and update their positions
        std::memmove(data + lowest - size, data + lowest, last - lowest);
        for (auto i = entry; i < nb; ++i)
            setPos(data, i, pos(data, i) - size);

        // Insert the new entry in the table
        std::memmove(data + entryOffset(entry + 1), data + entryOffset(entry), (nb - entry) * EntrySize);
        key.toBytes(data + entryOffset(entry));
        setPos(data, entry, last - size);
        std::memset(data + last - size, 0, size);
        set(data, Len, nb + 1);
        setFree(blockId, data, freeSpaces_[blockId - 1] - size - EntrySize);
        blocks_.blockModified();
    }

    /**
     * Resize the value of an entry (the block must have enough free space). The content of the value is not kept.
     * @param blockId           Block identifier.
     * @param data              Data representing the block.
     * @param entry             Entry index.
     * @param delta             Delta size.
     */
    void resize(const std::size_t blockId, char* data, const std::size_t entry, const std::ptrdiff_t delta)
    {
        if (!delta)
            return;
        const auto nb = get(data, Len);
        const auto start = pos(data, entry);
        const auto lowest = pos(data, nb - 1);

        // Move the values of the next entries and update the positions
        std::memmove(data + static_cast<std::ptrdiff_t>(lowest) - delta, data + lowest, start - lowest);
        for (auto i = entry; i < nb; ++i)
            setPos(data, i, static_cast<std::size_t>(static_cast<std::ptrdiff_t>(pos(data, i)) - delta));
        setFree(blockId, data, static_cast<std::size_t>(static_cast<std::ptrdiff_t>(freeSpaces_[blockId - 1]) - delta));
        blocks_.blockModified();
    }

    //-----------------------------------------------------
    RBlockFile blocks_;                     ///< Blocks of the file.
    std::size_t tolerance_;                 ///< Minimal free space of a block to receive a new pair.
    std::vector<std::size_t> freeSpaces_;   ///< Free space of each block.
    std::size_t recordBlock_ = 0;           ///< Block of the current value (0 if there is none).
    std::size_t recordStart_ = 0;           ///< Start of the current value.
    std::size_t recordEnd_ = 0;             ///< End of the current value.

    //-----------------------------------------------------
    static constexpr std::size_t Len = sizeof(std::uint64_t);       ///< Size of a number stored in a block (free space, number of pairs, positions).
    static constexpr std::size_t Len2 = 2 * Len;                    ///< Size of the header of a block.
    static constexpr std::size_t EntrySize = K::size + Len;         ///< Size of an entry in the address table.
};


}  //-------- End of namespace evocomb ---------------------------------------------
