/*

	EvoComb

	RBlockFile.hpp

	Block File - Header.

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
#include <filesystem>
#include <memory>
#include <vector>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "io/r_io_file.hpp"


//------------------------------------------------------------------------------
namespace evocomb{
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
 * The RBlockFile class represents a file made of blocks of a fixed size, identified by a number starting from 1 (0
 * represents no block). A cache keeps a given number of blocks in memory: when it is full, the block with the fewest
 * accesses is replaced (and saved if it was modified).
 *
 * Data are read and written at a position in a block (see seek): a read or a write cannot cross the end of a block.
 *
 * @code
 * char buffer[80];
 * evocomb::RBlockFile file("/tmp/test.bin", 1024, 2);   // Blocks of 1 KB, 2 blocks in cache
 * file.open();
 *
 * // Write data
 * file.seek(2, 20);                                 // Position 20 in the second block
 * file.write("hello", 5);
 *
 * // Read data
 * file.seek(2, 20);
 * file.read(buffer, 5);
 * @endcode
 *
 * The file starts with a header of 2 KB: its first byte is the size of a std::size_t (8 on 64 bits platforms), and the
 * next bytes are the size of the blocks (a std::size_t). The rest of the header is unused.
 * @warning A file created on a given architecture cannot be opened on another one.
 * @short Block File.
 */
class RBlockFile
{
	struct Data;

public:

    /**
     * The CacheType enum represents the way the modified blocks are saved.
     */
    enum class CacheType
    {
        WriteThrough,                       ///< Each write is also done on the disk.
        WriteBack                           ///< The modified blocks are saved when they leave the cache (or at flush).
    };

    /**
     * Size of the header of the file (in bytes).
     */
    static constexpr std::size_t HeaderSize = 2048;

    /**
     * Size of the blocks of a new file if it is not specified (1 MB).
     */
    static constexpr std::size_t DefaultBlockSize = 1024 * 1024;

    /**
     * Construct a file whose block size is defined by the file itself (or DefaultBlockSize for a new file).
     * @param path               Path of the file.
     * @param nbCaches           Number of blocks kept in memory.
     * @exception RIOException   The number of blocks in the cache is null.
     */
    RBlockFile(std::filesystem::path path, std::size_t nbCaches);

    /**
     * Construct a file. If the file exists, its block size must be the one given.
     * @param path               Path of the file.
     * @param blockSize          Size of a block (in bytes).
     * @param nbCaches           Number of blocks kept in memory.
     * @exception RIOException   The block size or the number of blocks in the cache is null.
     */
    RBlockFile(std::filesystem::path path, std::size_t blockSize, std::size_t nbCaches);

    /**
     * Destruct the file (the modified blocks are saved, the errors are ignored).
     */
    ~RBlockFile();

    /**
     * Get the path of the file.
     */
    [[nodiscard]] const std::filesystem::path& path() const { return file_.path(); }

    /**
     * Look if the file is open.
     */
    [[nodiscard]] bool isOpen() const { return file_.isOpen(); }

    /**
     * Open the file (in read and write mode, the only possible one). It is created if necessary.
     * @exception RIOException   The file cannot be open, or it was created with another architecture or block size.
     */
    void open();

    /**
     * Close the file (the modified blocks are saved).
     */
    void close();

    /**
     * Remove all the blocks from the file.
     */
    void clear();

    /**
     * Get the size of the blocks.
     */
    [[nodiscard]] std::size_t blockSize() const { return blockSize_; }

    /**
     * Get the number of blocks.
     */
    [[nodiscard]] std::size_t nbBlocks() const { return nbBlocks_; }

    /**
     * Get the type of the cache.
     */
    [[nodiscard]] CacheType cacheType() const { return cacheType_; }

    /**
     * Set the type of the cache. If the type becomes CacheType::WriteThrough, the modified blocks are saved.
     * @param type               Type.
     */
    void cacheType(CacheType type);

    /**
     * Save all the modified blocks of the cache on the disk.
     */
    void flush();

    /**
     * Read a given number of bytes at the current position.
     * @param buffer             Buffer (it must contain at least nb bytes).
     * @param nb                 Number of bytes to read.
     * @exception RIOException   There is no current block or the read crosses the end of the block.
     */
    void read(char* buffer, std::size_t nb);

    /**
     * Write a given number of bytes at the current position.
     * @param buffer             Buffer.
     * @param nb                 Number of bytes to write.
     * @exception RIOException   There is no current block or the write crosses the end of the block.
     */
    void write(const char* buffer, std::size_t nb);

    /**
     * Move a given number of bytes inside a block (the data at the destination are overwritten, and the two areas may
     * overlap). The current position is then the end of the moved bytes at the destination.
     * @param blockId            Identifier of the block.
     * @param start              Position of the bytes to move.
     * @param end                Destination of the bytes.
     * @param size               Number of bytes to move.
     */
    void moveBlock(std::size_t blockId, std::size_t start, std::size_t end, std::size_t size);

    /**
     * Get a pointer to the data at the current position, and move the current position after the given number of
     * bytes. The pointer is only valid until another block is loaded.
     * @param size               Number of bytes to access.
     */
    [[nodiscard]] const char* getPtr(std::size_t size);

    /**
     * Go to a given position in a given block (the block is created if it does not exist).
     * @param blockId            Identifier of the block.
     * @param pos                Position in the block.
     */
    void seek(std::size_t blockId, std::size_t pos);

    /**
     * Move from a given number of bytes in the current block.
     * @param rel                Number of bytes (it can be negative).
     */
    void seekRel(std::ptrdiff_t rel);

    /**
     * Get the identifier of the current block (0 if there is none).
     */
    [[nodiscard]] std::size_t currentBlock() const;

    /**
     * Get the current position in the current block.
     */
    [[nodiscard]] std::size_t currentPos() const { return currentPos_; }

private:

    /**
     * Get the data of a block (it becomes the current block, and the current position is 0). The pointer is only
     * valid until another block is loaded. If the data are modified, blockModified must be called.
     * @param blockId            Identifier of the block.
     */
    [[nodiscard]] char* blockData(std::size_t blockId);

    /**
     * Specify that the current block was modified (it is saved immediately in CacheType::WriteThrough mode).
     */
    void blockModified();

    /**
     * Get a block from the cache, or load it (a block that does not exist yet is created).
     * @param blockId            Identifier of the block.
     */
    Data* loadBlock(std::size_t blockId);

    /**
     * Save a block on the disk.
     * @param block              Block.
     */
    void saveBlock(Data& block);

    /**
     * Verify that the file is open and that a read or a write of a given number of bytes can be done in the current
     * block.
     * @param nb                 Number of bytes.
     */
    void verify(std::size_t nb) const;

    /**
     * Compute the position in the file of a byte of a block (the computation is done with std::streamoff values).
     * @param blockId            Identifier of the block.
     * @param pos                Position in the block.
     */
    [[nodiscard]] std::streamoff offset(std::size_t blockId, std::size_t pos = 0) const;

    //-----------------------------------------------------
    RIOFile file_;											///< File.
    std::size_t blockSize_;									///< Size of the blocks (0 if defined by the file).
    std::size_t nbCaches_;									///< Maximum number of blocks in the cache.
    CacheType cacheType_ = CacheType::WriteBack;			///< Type of the cache.
    std::vector<std::unique_ptr<Data>> cache_;				///< Blocks in memory.
    Data* current_ = nullptr;								///< Current block.
    std::size_t currentPos_ = 0;							///< Current position in the current block.
    std::size_t nbBlocks_ = 0;								///< Number of blocks.
    bool fixedSize_;										///< Is the size of the blocks imposed by the constructor?

	//-----------------------------------------------------
	template<class K> friend class RKeyValueFile;
};


}  //-------- End of namespace evocomb ---------------------------------------------
