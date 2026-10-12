/*

	EvoComb

	RIOFile.hpp

	Generic File for Input/Output - Header.

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
#include <fstream>
#include <ios>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "io/r_io.hpp"


//------------------------------------------------------------------------------
namespace evocomb{
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
 * The RIOFile class represents a binary file that can be read and/or written at any position. It is built on a
 * std::fstream (which manages the buffering), and it keeps its own "virtual" position: each read or write is done at
 * this position, which can be changed with seek and seekRel.
 *
 * The positions and the sizes in the file are std::streamoff (a signed 64 bits integer on all platforms), while the
 * numbers of bytes read or written in a memory buffer are std::size_t.
 *
 * Here is an example:
 * @code
 * #include "io/r_io_file.hpp"
 * using namespace EvoComb;
 *
 * char buffer[81];
 * RIOFile in("/home/user/Test.txt");
 * in.open(RIO::Read);
 * const auto len = in.read(buffer, 80);
 * buffer[len] = 0;
 * std::cout << buffer << std::endl;
 * @endcode
 *
 * The modes (see RIO::ModeType) are:
 * - RIO::Read: the file must exist and can only be read.
 * - RIO::Write: the file is created if necessary (but not emptied) and can only be written.
 * - RIO::ReadWrite: the file is created if necessary (but not emptied) and can be read and written.
 * - RIO::Append: as RIO::ReadWrite, but the position is at the end of the file and every write is done at the end
 *   of the file (whatever the current position).
 * - RIO::Create: the file is created or emptied and can only be written.
 *
 * A position beyond the end of the file can only be reached if the file can be written: the bytes between the end of
 * the file and the position are then filled with 0 at the next write.
 * @short Generic File for Input/Output.
 */
class RIOFile
{
public:

    /**
     * Construct a file without path (see open).
     */
    RIOFile() = default;

    /**
     * Construct a file.
     * @param path               Path of the file.
     */
    explicit RIOFile(std::filesystem::path path);

    /**
     * Destruct the file (it is closed if necessary, the errors are ignored).
     */
    virtual ~RIOFile();

    /**
     * Get the path of the file.
     */
    [[nodiscard]] const std::filesystem::path& path() const { return path_; }

    /**
     * Open the file. If it is already open, it is first closed.
     * @param mode               Open mode.
     * @exception RIOException   The file cannot be open.
     */
    virtual void open(RIO::ModeType mode);

    /**
     * Open a file. If a file is already open, it is first closed.
     * @param path               Path of the file.
     * @param mode               Open mode.
     * @exception RIOException   The file cannot be open.
     */
    virtual void open(const std::filesystem::path& path, RIO::ModeType mode);

    /**
     * Get the open mode (RIO::Undefined if the file is not open).
     */
    [[nodiscard]] RIO::ModeType mode() const { return mode_; }

    /**
     * Look if the file is open.
     */
    [[nodiscard]] bool isOpen() const { return file_.is_open(); }

    /**
     * Close the file (nothing is done if it is not open).
     * @exception RIOException   The file cannot be closed (for example, the pending writes failed).
     */
    virtual void close();

    /**
     * Read a given number of bytes at the current position of the file.
     * @param buffer             Buffer (it must contain at least nb bytes).
     * @param nb                 Number of bytes to read. If there are less bytes left in the file, only the remaining
     *                           bytes are read.
     * @param move               Is the current position moved after the bytes read (default)? If not, the next read
     *                           reads the same bytes.
     * @return the number of bytes read.
     * @exception RIOException   The file is not open for reading, the end of the file is reached, or the read failed.
     */
    std::size_t read(char* buffer, std::size_t nb, bool move = true);

    /**
     * Write a given number of bytes at the current position of the file (or at the end of the file in RIO::Append
     * mode). The current position is moved after the bytes written.
     * @param buffer             Buffer.
     * @param nb                 Number of bytes to write.
     * @exception RIOException   The file is not open for writing or the write failed.
     */
    void write(const char* buffer, std::size_t nb);

    /**
     * Go to a given position of the file.
     * @param pos                Position.
     * @exception RIOException   The file is not open, the position is negative, or the position is beyond the end of
     *                           the file while the file cannot be written.
     */
    virtual void seek(std::streamoff pos);

    /**
     * Move from a given number of bytes from the current position of the file.
     * @param rel                Number of bytes (it can be negative).
     * @exception RIOException   The new position is invalid (see seek).
     */
    virtual void seekRel(std::streamoff rel);

    /**
     * Go to the end of the file (just after the last byte, where end returns `true`).
     * @exception RIOException   The file is not open.
     */
    virtual void seekToEnd();

    /**
     * Truncate (or extend with 0) the file to a given size. If the current position is beyond the new size, it is
     * moved to the end of the file.
     * @param newSize            New size.
     * @exception RIOException   The file is not open for writing, the size is negative or the operation failed.
     */
    void truncate(std::streamoff newSize);

    /**
     * Look if the current position is at (or beyond) the end of the file.
     */
    [[nodiscard]] bool end() const { return pos_ >= size_; }

    /**
     * Get the size of the file.
     */
    [[nodiscard]] std::streamoff size() const { return size_; }

    /**
     * Get the current position in the file.
     */
    [[nodiscard]] std::streamoff pos() const { return pos_; }

    //-----------------------------------------------------
    RIOFile(const RIOFile&) = delete;
    RIOFile& operator=(const RIOFile&) = delete;

private:

    /**
     * Last operation done on the stream (a seek is necessary between a read and a write).
     */
    enum class Operation { None, Read, Write };

    /**
     * Position the stream at the current position if necessary (the stream is not positioned if it is already at the
     * current position and the previous operation was of the same type, in order to keep its buffer).
     * @param op                 Operation that will be done.
     */
    void prepare(Operation op);

    //-----------------------------------------------------
    std::filesystem::path path_;            ///< Path of the file.
    std::fstream file_;                     ///< Stream.
    RIO::ModeType mode_ = RIO::Undefined;   ///< Open mode.
    bool canRead_ = false;                  ///< Can the file be read?
    bool canWrite_ = false;                 ///< Can the file be written?
    std::streamoff size_ = 0;               ///< Size of the file.
    std::streamoff pos_ = 0;                ///< Current (virtual) position in the file.
    std::streamoff streamPos_ = -1;         ///< Position of the stream (-1 if it is unknown).
    Operation lastOp_ = Operation::None;    ///< Last operation done on the stream.
};


}  //-------- End of namespace evocomb ---------------------------------------------
