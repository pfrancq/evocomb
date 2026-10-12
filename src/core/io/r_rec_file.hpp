/*

	EvoComb

	RRecFile.hpp

	Binary file for records - Header.

	Copyright 2002-2026 by Pascal Francq (pascal@francq.info).
	Copyright 2002-2008 by the Université Libre de Bruxelles (ULB).

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
#include <ios>
#include <utility>
#include <vector>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "io/r_binary_file.hpp"
#include "app/r_logger.hpp"


//------------------------------------------------------------------------------
namespace evocomb{
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
 * The RRecFile class represents a binary file made of records of a fixed size. The records can be read sequentially
 * (as with a cursor), or directly through their number. If the records are ordered in the file (bOrder is `true`), a
 * record can be searched with a binary search (see search).
 *
 * The class of the records must be default-constructible and provide the methods reading and writing a record at the
 * current position of the file (a record may use less bytes than the size of the records, but not more):
 * @code
 * void read(evocomb::RRecFile<C, bOrder>& file);
 * void write(evocomb::RRecFile<C, bOrder>& file) const;
 * @endcode
 * To use search, it must also provide a method comparing a record with a tag:
 * @code
 * int compare(const TUse& tag) const;     // < 0, 0 or > 0 if the record is before, equal or after the tag
 * @endcode
 *
 * Here is an example:
 * @code
 * class Person
 * {
 * public:
 *     std::uint32_t id = 0;
 *     double salary = 0.0;
 *
 *     void read(evocomb::RRecFile<Person, true>& file) { file >> id >> salary; }
 *     void write(evocomb::RRecFile<Person, true>& file) const { file << id << salary; }
 *     int compare(const std::uint32_t tag) const { return (id < tag) ? -1 : (id > tag); }
 * };
 *
 * evocomb::RRecFile<Person, true> file("/tmp/persons.bin", sizeof(std::uint32_t) + sizeof(double));
 * file.open(evocomb::RIO::Create);
 * file.writeRec(Person{1, 1000.0});
 * file.writeRec(Person{5, 2000.0});
 * file.close();
 *
 * file.open(evocomb::RIO::Read);
 * for (file.start(); !file.end(); file.next())
 *     std::cout << file()->id << std::endl;
 * file.search(5u);
 * if (file.found())
 *     std::cout << file()->salary << std::endl;
 * @endcode
 * @tparam C                     Class of the records.
 * @tparam bOrder                Are the records ordered in the file?
 * @short Fixed-length Records File.
 */
template<class C, bool bOrder = false>
    class RRecFile : public RBinaryFile
{
public:

    /**
     * Construct a file without path (see open).
     */
    RRecFile() = default;

    /**
     * Construct a file.
     * @param path               Path of the file.
     * @param recSize            Size of a record (in bytes).
     */
    RRecFile(std::filesystem::path path, const std::size_t recSize) :
        RBinaryFile(std::move(path)), recSize_(recSize) {}

    using RBinaryFile::open;

    /**
     * Open the file (the current position is the first record, or the end of the file in RIO::Append mode).
     * @param mode               Open mode.
     * @exception RIOException   The file cannot be open or the size of the records is null.
     */
    void open(const RIO::ModeType mode) override
    {
        mLogModule("r.core");
        if (recSize_ == 0)
            mThrowRIOException(this, "The size of the records is null");
        found_ = false;
        loaded_ = false;
        RBinaryFile::open(mode);
    }

    /**
     * Open a file.
     * @param path               Path of the file.
     * @param recSize            Size of a record (in bytes).
     * @param mode               Open mode.
     * @exception RIOException   The file cannot be open or the size of the records is null.
     */
    void open(const std::filesystem::path& path, const std::size_t recSize, const RIO::ModeType mode = RIO::Read)
    {
        recSize_ = recSize;
        RBinaryFile::open(path, mode);
    }

    /**
     * Close the file.
     */
    void close() override
    {
        found_ = false;
        loaded_ = false;
        RBinaryFile::close();
    }

    /**
     * Go to a given position of the file (the current record is invalidated).
     * @param pos                Position.
     */
    void seek(const std::streamoff pos) override
    {
        loaded_ = false;
        RBinaryFile::seek(pos);
    }

    /**
     * Get the size of a record (in bytes).
     */
    [[nodiscard]] std::size_t recSize() const { return recSize_; }

    /**
     * Get the number of records in the file.
     */
    [[nodiscard]] std::size_t recNb() const { return static_cast<std::size_t>(size()) / recSize_; }

    /**
     * Get the number of the current record.
     */
    [[nodiscard]] std::size_t currentRec() const { return static_cast<std::size_t>(pos()) / recSize_; }

    /**
     * Go to a given record.
     * @param nb                 Number of the record.
     */
    void goToRec(const std::size_t nb) { seek(static_cast<std::streamoff>(nb * recSize_)); }

    /**
     * Go to a given record when the file represents a matrix stored line by line.
     * @param col                Column.
     * @param line               Line.
     * @param nbCols             Number of columns.
     */
    void goToRec(const std::size_t col, const std::size_t line, const std::size_t nbCols) { goToRec(col + line * nbCols); }

    /**
     * Search for a record with a binary search (the records must be ordered). If the record is found, it becomes the
     * current record, else the current position is where the record should be inserted.
     * @tparam TUse              Type of the tag (C must provide an int compare(const TUse&) const method).
     * @param tag                Tag.
     * @return the number of the record if it is found (see found), or the number where it should be inserted.
     */
    template<class TUse>
        std::size_t search(const TUse& tag)
    {
        static_assert(bOrder, "search can only be used with ordered records");
        std::size_t min = 0;
        std::size_t max = recNb();
        while (min < max)
        {
            const auto mid = min + (max - min) / 2;
            goToRec(mid);
            const auto comp = (*this)()->compare(tag);
            if (comp == 0)
            {
                found_ = true;
                return mid;
            }
            if (comp < 0)
                min = mid + 1;
            else
                max = mid;
        }
        found_ = false;
        goToRec(min);
        return min;
    }

    /**
     * Look if the last search found the record.
     */
    [[nodiscard]] bool found() const { return found_; }

    /**
     * Read the record at the current position, and go to the next record.
     * @param rec                Record.
     * @exception RIOException   There is no record at the current position, or the record read more bytes than the
     *                           size of the records.
     */
    void readRec(C& rec)
    {
        mLogModule("r.core");
        const auto begin = pos();
        const auto next = begin + static_cast<std::streamoff>(recSize_);
        if (next > size())
            mThrowRIOException(this, "No record at position ", begin);
        rec.read(*this);
        if (pos() > next)
            mThrowRIOException(this, "The record read ", pos() - begin, " bytes instead of ", recSize_);
        seek(next);
    }

    /**
     * Write a record at the current position (the unused bytes are filled with 0), and go to the next record.
     * @param rec                Record.
     * @exception RIOException   The record wrote more bytes than the size of the records (the bytes are written, and
     *                           the next record is therefore corrupted).
     */
    void writeRec(const C& rec)
    {
        mLogModule("r.core");
        const auto begin = mode() == RIO::Append ? size() : pos();
        seek(begin);
        rec.write(*this);
        const auto written = pos() - begin;
        if (written > static_cast<std::streamoff>(recSize_))
            mThrowRIOException(this, "The record wrote ", written, " bytes instead of ", recSize_);
        if (const auto left = recSize_ - static_cast<std::size_t>(written); left)
        {
            const std::vector<char> zeros(left, 0);
            write(zeros.data(), left);
        }
        loaded_ = false;
    }

    /**
     * Go to the first record.
     */
    void start() { seek(0); }

    /**
     * Go to the next record.
     */
    void next() { seekRel(static_cast<std::streamoff>(recSize_)); }

    /**
     * Go to the previous record.
     */
    void prev() { seekRel(-static_cast<std::streamoff>(recSize_)); }

    /**
     * Get the current record (the current position is not modified).
     * @return a pointer to the record, or nullptr if there is no record at the current position.
     */
    C* operator()()
    {
        if (pos() + static_cast<std::streamoff>(recSize_) > size())
            return nullptr;
        if (!loaded_)
        {
            const auto begin = pos();
            readRec(current_);
            seek(begin);
            loaded_ = true;
        }
        return &current_;
    }

private:

    //-----------------------------------------------------
    std::size_t recSize_ = 0;               ///< Size of a record.
    C current_{};                           ///< Current record (see operator()).
    bool loaded_ = false;                   ///< Is the current record loaded?
    bool found_ = false;                    ///< Was the last searched record found?
};


}  //-------- End of namespace evocomb ---------------------------------------------
