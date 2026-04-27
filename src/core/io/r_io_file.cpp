/*

	EvoComb

	RIOFile.cpp

	Generic File for Input/Output - Implementation.

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
#include <cerrno>
#include <string>
#include <system_error>
#include <utility>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "io/r_io_file.hpp"
#include "app/r_logger.hpp"
using namespace evocomb;


//------------------------------------------------------------------------------
mLogModule("r.core");


//------------------------------------------------------------------------------
/**
 * Get the message of the last system error (the standard streams do not report the reason of an error, but errno is
 * set in practice by the implementations).
 */
static std::string lastError()
{
    if (errno == 0)
        return "unknown error";
    return std::generic_category().message(errno);
}



//------------------------------------------------------------------------------
//
// class RIOFile
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
RIOFile::RIOFile(std::filesystem::path path) :
    path_(std::move(path))
{
}


//------------------------------------------------------------------------------
RIOFile::~RIOFile()
{
    try
    {
        RIOFile::close();
    }
    catch (...)
    {
    }
}


//------------------------------------------------------------------------------
void RIOFile::open(const RIO::ModeType mode)
{
    if (isOpen())
        close();
    if (path_.empty())
        mThrowRIOException(this, "No path for the file");

    // Flags of the stream (in | out never empties the file, but the file must exist)
    std::ios::openmode flags = std::ios::binary;
    bool mustExist = false;
    switch (mode)
    {
        case RIO::Read:
            flags |= std::ios::in;
            canRead_ = true;
            canWrite_ = false;
            mustExist = true;
            break;
        case RIO::Write:
            flags |= std::ios::in | std::ios::out;
            canRead_ = false;
            canWrite_ = true;
            break;
        case RIO::ReadWrite:
        case RIO::Append:
            flags |= std::ios::in | std::ios::out;
            canRead_ = true;
            canWrite_ = true;
            break;
        case RIO::Create:
            flags |= std::ios::in | std::ios::out | std::ios::trunc;
            canRead_ = false;
            canWrite_ = true;
            break;
        default:
            mThrowRIOException(this, "No valid mode");
    }

    std::error_code ec;
    const bool exists = std::filesystem::exists(path_, ec);
    if (mustExist && !exists)
        mThrowRIOException(this, "The file does not exist");
    if (exists && std::filesystem::is_directory(path_, ec))
        mThrowRIOException(this, "The file is a directory");

    // Create the file if necessary (in | out cannot create it)
    if (!exists && mode != RIO::Create && mode != RIO::Read)
    {
        errno = 0;
        if (std::ofstream(path_, std::ios::binary | std::ios::out).fail())
            mThrowRIOException(this, "Cannot create the file: ", lastError());
    }

    // Open the stream
    errno = 0;
    file_.open(path_, flags);
    if (!file_.is_open())
        mThrowRIOException(this, "Cannot open the file: ", lastError());

    // Size of the file
    file_.seekg(0, std::ios::end);
    size_ = file_.tellg();
    if (size_ < 0)
    {
        file_.close();
        mThrowRIOException(this, "Cannot get the size of the file");
    }
    mode_ = mode;
    pos_ = (mode == RIO::Append) ? size_ : 0;
    streamPos_ = -1;
    lastOp_ = Operation::None;
}


//------------------------------------------------------------------------------
void RIOFile::open(const std::filesystem::path& path, const RIO::ModeType mode)
{
    if (isOpen())
        close();
    path_ = path;
    open(mode);
}


//------------------------------------------------------------------------------
void RIOFile::close()
{
    if (!isOpen())
        return;
    errno = 0;
    file_.close();
    const bool failed = file_.fail();
    file_.clear();
    mode_ = RIO::Undefined;
    canRead_ = false;
    canWrite_ = false;
    size_ = 0;
    pos_ = 0;
    streamPos_ = -1;
    lastOp_ = Operation::None;
    if (failed)
        mThrowRIOException(this, "Cannot close the file: ", lastError());
}


//------------------------------------------------------------------------------
void RIOFile::prepare(const Operation op)
{
    if (op == lastOp_ && streamPos_ == pos_)
        return;
    file_.clear();
    if (op == Operation::Read)
        file_.seekg(pos_);
    else
        file_.seekp(pos_);
    if (file_.fail())
        mThrowRIOException(this, "Cannot move to position ", pos_);
    streamPos_ = pos_;
    lastOp_ = op;
}


//------------------------------------------------------------------------------
std::size_t RIOFile::read(char* buffer, std::size_t nb, const bool move)
{
    if (!isOpen())
        mThrowRIOException(this, "Can't read in the file");
    if (!canRead_)
        mThrowRIOException(this, "No read access");
    if (end())
        mThrowRIOException(this, "End of the file reached");

    // Only read the bytes left
    if (static_cast<std::streamoff>(nb) > size_ - pos_)
        nb = static_cast<std::size_t>(size_ - pos_);
    if (nb == 0)
        return 0;

    prepare(Operation::Read);
    errno = 0;
    file_.read(buffer, static_cast<std::streamsize>(nb));
    const auto total = file_.gcount();
    if (file_.bad() || total != static_cast<std::streamsize>(nb))
    {
        streamPos_ = -1;
        mThrowRIOException(this, "Cannot read ", nb, " bytes: ", lastError());
    }
    streamPos_ = pos_ + total;
    if (move)
        pos_ = streamPos_;
    return static_cast<std::size_t>(total);
}


//------------------------------------------------------------------------------
void RIOFile::write(const char* buffer, const std::size_t nb)
{
    if (!isOpen())
        mThrowRIOException(this, "Can't write into the file");
    if (!canWrite_)
        mThrowRIOException(this, "No write access");
    if (nb == 0)
        return;

    if (mode_ == RIO::Append)
        pos_ = size_;
    prepare(Operation::Write);
    errno = 0;
    file_.write(buffer, static_cast<std::streamsize>(nb));
    if (file_.fail())
    {
        streamPos_ = -1;
        mThrowRIOException(this, "Cannot write ", nb, " bytes: ", lastError());
    }
    pos_ += static_cast<std::streamoff>(nb);
    streamPos_ = pos_;
    if (pos_ > size_)
        size_ = pos_;
}


//------------------------------------------------------------------------------
void RIOFile::seek(const std::streamoff pos)
{
    if (!isOpen())
        mThrowRIOException(this, "Can't seek the file");
    if (pos < 0)
        mThrowRIOException(this, "Position before the beginning of the file");
    if (pos > size_ && !canWrite_)
        mThrowRIOException(this, "Position ", pos, " outside of the file");
    pos_ = pos;
}


//------------------------------------------------------------------------------
void RIOFile::seekRel(const std::streamoff rel)
{
    seek(pos_ + rel);
}


//------------------------------------------------------------------------------
void RIOFile::seekToEnd()
{
    seek(size_);
}


//------------------------------------------------------------------------------
void RIOFile::truncate(const std::streamoff newSize)
{
    if (!isOpen())
        mThrowRIOException(this, "Can't truncate the file");
    if (!canWrite_)
        mThrowRIOException(this, "No write access");
    if (newSize < 0)
        mThrowRIOException(this, "New size is negative");

    // The pending writes must be done before the file is resized
    file_.flush();
    if (file_.fail())
        mThrowRIOException(this, "Cannot write the pending bytes: ", lastError());
    std::error_code ec;
    std::filesystem::resize_file(path_, static_cast<std::uintmax_t>(newSize), ec);
    if (ec)
        mThrowRIOException(this, "Cannot truncate the file: ", ec.message());

    // The buffer of the stream may be invalid: the stream must be positioned again
    size_ = newSize;
    if (pos_ > size_)
        pos_ = size_;
    streamPos_ = -1;
    lastOp_ = Operation::None;
}
