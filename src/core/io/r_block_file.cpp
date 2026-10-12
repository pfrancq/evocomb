/*

	EvoComb

	RBlockFile.cpp

	Block File - Implementation.

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
// include files for ANSI C/C++
#include <algorithm>
#include <cstring>
#include <utility>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "io/r_block_file.hpp"
#include "app/r_logger.hpp"
using namespace evocomb;


//------------------------------------------------------------------------------
mLogModule("r.core");


//------------------------------------------------------------------------------
//
// class class RBlockFile::Data
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
struct RBlockFile::Data
{
    Data(const std::size_t id, const std::size_t size) : id_(id), data_(size, 0) {}
    std::size_t id_;                        ///< Identifier of the block.
    std::size_t nbAccess_ = 0;              ///< Number of accesses since the block was loaded.
    bool dirty_ = false;                    ///< Was the block modified since it was loaded or saved?
    std::vector<char> data_;                ///< Data of the block.
};



//------------------------------------------------------------------------------
//
// class RBlockFile
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
RBlockFile::RBlockFile(std::filesystem::path path, const std::size_t nbCaches) :
    file_(std::move(path)), blockSize_(DefaultBlockSize), nbCaches_(nbCaches), fixedSize_(false)
{
    if (!nbCaches_)
        mThrowRIOException(&file_, "The number of blocks in the cache cannot be null");
}


//------------------------------------------------------------------------------
RBlockFile::RBlockFile(std::filesystem::path path, const std::size_t blockSize, const std::size_t nbCaches) :
    file_(std::move(path)), blockSize_(blockSize), nbCaches_(nbCaches), fixedSize_(true)
{
    if (!blockSize_)
        mThrowRIOException(&file_, "The block size cannot be null");
    if (!nbCaches_)
        mThrowRIOException(&file_, "The number of blocks in the cache cannot be null");
}


//------------------------------------------------------------------------------
RBlockFile::~RBlockFile()
{
    try
    {
        close();
    }
    catch (...)
    {
    }
}


//------------------------------------------------------------------------------
void RBlockFile::open()
{
    file_.open(RIO::ReadWrite);
    cache_.clear();
    current_ = nullptr;
    currentPos_ = 0;

    constexpr auto arch = static_cast<char>(sizeof(std::size_t));
    if (file_.size() == 0)
    {
        // New file: write the header
        std::vector<char> header(HeaderSize, 0);
        header[0] = arch;
        std::memcpy(&header[1], &blockSize_, sizeof(std::size_t));
        file_.write(header.data(), header.size());
    }
    else
    {
        char fileArch = 0;
        std::size_t fileSize = 0;
        if (file_.size() < static_cast<std::streamoff>(HeaderSize))
            mThrowRIOException(&file_, "The file is not a block file");
        file_.seek(0);
        file_.read(&fileArch, 1);
        if (fileArch != arch)
            mThrowRIOException(&file_, "The file was created on another architecture");
        file_.read(reinterpret_cast<char*>(&fileSize), sizeof(std::size_t));
        if (!fileSize)
            mThrowRIOException(&file_, "The file is not a block file");
        if (fixedSize_ && fileSize != blockSize_)
            mThrowRIOException(&file_, "The block size of the file (", fileSize, ") is not ", blockSize_);
        blockSize_ = fileSize;
    }
    nbBlocks_ = (static_cast<std::size_t>(file_.size()) - HeaderSize + blockSize_ - 1) / blockSize_;
}


//------------------------------------------------------------------------------
void RBlockFile::close()
{
    if (!isOpen())
        return;
    flush();
    cache_.clear();
    current_ = nullptr;
    currentPos_ = 0;
    nbBlocks_ = 0;
    file_.close();
}


//------------------------------------------------------------------------------
void RBlockFile::clear()
{
    if (!isOpen())
        mThrowRIOException(&file_, "File not opened");
    cache_.clear();
    current_ = nullptr;
    currentPos_ = 0;
    nbBlocks_ = 0;
    file_.truncate(HeaderSize);
}


//------------------------------------------------------------------------------
void RBlockFile::cacheType(const CacheType type)
{
    const auto old = cacheType_;
    cacheType_ = type;
    if (isOpen() && old == CacheType::WriteBack && type == CacheType::WriteThrough)
        flush();
}


//------------------------------------------------------------------------------
void RBlockFile::flush()
{
    if (!isOpen())
        mThrowRIOException(&file_, "File not opened");
    for (const auto& block : cache_)
        if (block->dirty_)
            saveBlock(*block);
}


//------------------------------------------------------------------------------
void RBlockFile::saveBlock(Data& block)
{
    file_.seek(offset(block.id_));
    file_.write(block.data_.data(), blockSize_);
    block.dirty_ = false;
}


//------------------------------------------------------------------------------
RBlockFile::Data* RBlockFile::loadBlock(const std::size_t blockId)
{
    if (!isOpen())
        mThrowRIOException(&file_, "File not opened");
    if (!blockId)
        mThrowRIOException(&file_, "There is no block with a null identifier");

    // Search for the block in memory
    const auto same = [blockId](const auto& b) { return b->id_ == blockId; };
    if (const auto it = std::find_if(cache_.begin(), cache_.end(), same); it != cache_.end())
        return it->get();

    // Find a place in the cache: a new block, or the one with the fewest accesses (saved if necessary)
    Data* block;
    if (cache_.size() < nbCaches_)
    {
        cache_.push_back(std::make_unique<Data>(blockId, blockSize_));
        block = cache_.back().get();
    }
    else
    {
        block = std::min_element(cache_.begin(), cache_.end(),
                                 [](const auto& a, const auto& b) { return a->nbAccess_ < b->nbAccess_; })->get();
        if (block->dirty_)
            saveBlock(*block);
        if (block == current_)
            current_ = nullptr;
        block->id_ = blockId;
        block->nbAccess_ = 0;
        std::fill(block->data_.begin(), block->data_.end(), 0);
    }

    // Read the data (the bytes beyond the end of the file are 0). A new block must be saved, even if it is not
    // modified, to keep the file size coherent with the number of blocks.
    if (const auto start = offset(blockId); start < file_.size())
    {
        file_.seek(start);
        file_.read(block->data_.data(), std::min(blockSize_, static_cast<std::size_t>(file_.size() - start)));
    }
    if (blockId > nbBlocks_)
    {
        nbBlocks_ = blockId;
        block->dirty_ = true;
        if (cacheType_ == CacheType::WriteThrough)
            saveBlock(*block);
    }
    return block;
}


//------------------------------------------------------------------------------
void RBlockFile::verify(const std::size_t nb) const
{
    if (!isOpen())
        mThrowRIOException(&file_, "File not opened");
    if (!current_)
        mThrowRIOException(&file_, "No current block");
    if (currentPos_ + nb > blockSize_)
        mThrowRIOException(&file_, "Size of a block is limited to ", blockSize_);
}


//------------------------------------------------------------------------------
void RBlockFile::read(char* buffer, const std::size_t nb)
{
    verify(nb);
    std::memcpy(buffer, current_->data_.data() + currentPos_, nb);
    currentPos_ += nb;
    current_->nbAccess_++;
}


//------------------------------------------------------------------------------
void RBlockFile::write(const char* buffer, const std::size_t nb)
{
    verify(nb);
    std::memcpy(current_->data_.data() + currentPos_, buffer, nb);
    if (cacheType_ == CacheType::WriteThrough)
    {
        file_.seek(offset(current_->id_, currentPos_));
        file_.write(buffer, nb);
    }
    else
        current_->dirty_ = true;
    currentPos_ += nb;
    current_->nbAccess_++;
}


//------------------------------------------------------------------------------
void RBlockFile::moveBlock(const std::size_t blockId, const std::size_t start, const std::size_t end,
                           const std::size_t size)
{
    if (start + size > blockSize_ || end + size > blockSize_)
        mThrowRIOException(&file_, "Size of a block is limited to ", blockSize_);
    seek(blockId, end + size);
    std::memmove(current_->data_.data() + end, current_->data_.data() + start, size);
    blockModified();
}


//------------------------------------------------------------------------------
const char* RBlockFile::getPtr(const std::size_t size)
{
    verify(size);
    const char* ptr = current_->data_.data() + currentPos_;
    currentPos_ += size;
    current_->nbAccess_++;
    return ptr;
}


//------------------------------------------------------------------------------
void RBlockFile::seek(const std::size_t blockId, const std::size_t pos)
{
    if (pos > blockSize_)
        mThrowRIOException(&file_, "Size of a block is limited to ", blockSize_);
    if (!current_ || current_->id_ != blockId)
        current_ = loadBlock(blockId);
    currentPos_ = pos;
}


//------------------------------------------------------------------------------
void RBlockFile::seekRel(const std::ptrdiff_t rel)
{
    if (!current_)
        mThrowRIOException(&file_, "No current block");
    const auto pos = static_cast<std::ptrdiff_t>(currentPos_) + rel;
    if (pos < 0 || pos > static_cast<std::ptrdiff_t>(blockSize_))
        mThrowRIOException(&file_, "Relative position outside the block");
    currentPos_ = static_cast<std::size_t>(pos);
}


//------------------------------------------------------------------------------
std::size_t RBlockFile::currentBlock() const
{
    return current_ ? current_->id_ : 0;
}


//------------------------------------------------------------------------------
char* RBlockFile::blockData(const std::size_t blockId)
{
    seek(blockId, 0);
    current_->nbAccess_++;
    return current_->data_.data();
}


//------------------------------------------------------------------------------
void RBlockFile::blockModified()
{
    if (!current_)
        mThrowRIOException(&file_, "No current block");
    if (cacheType_ == CacheType::WriteThrough)
        saveBlock(*current_);
    else
        current_->dirty_ = true;
}


//------------------------------------------------------------------------------
std::streamoff RBlockFile::offset(const std::size_t blockId, const std::size_t pos) const
{
    return static_cast<std::streamoff>(HeaderSize) +
           static_cast<std::streamoff>(blockId - 1) * static_cast<std::streamoff>(blockSize_) +
           static_cast<std::streamoff>(pos);
}
