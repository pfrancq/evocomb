/*

	EvoComb

	RCSVFile.hpp

	CSV File - Header.

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
#include <istream>
#include <locale>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "io/r_io.hpp"
#include "app/r_logger.hpp"


//------------------------------------------------------------------------------
namespace evocomb{
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
 * The RCSVFile class reads a CSV (Comma-Separated Values) file, record by record:
 * - The values are separated by a given character (a comma by default).
 * - A value can be delimited by double quotes (if this option is set): it can then contain the separator, line breaks
 *   and double quotes (written as two double quotes). The spaces around the quotes are ignored.
 * - The spaces at the beginning and at the end of a value that is not delimited by quotes are removed.
 * - An escape character (a backslash by default, `false` to disable it) adds the next character to the value whatever
 *   it is (for compatibility with the old R library, since it is not part of the CSV format).
 * - The empty lines are skipped, and a line ending with a separator has a last empty value.
 * - The file is read as bytes (UTF-8 is supported, a UTF-8 byte order mark is skipped).
 *
 * Here is an example:
 * @code
 * evocomb::RCSVFile file("/home/user/data.csv");
 * file.open();
 * while (!file.end())
 * {
 *     file.read();
 *     std::cout << file.value(0) << ": " << file.value<double>(1) << std::endl;
 * }
 * @endcode
 * @short CSV File.
 */
class RCSVFile
{
public:

    /**
     * Construct a CSV file without path (see open and parse).
     * @param sep                Separator.
     * @param quotes             Can the values be delimited by double quotes?
     */
    explicit RCSVFile(char sep = ',', bool quotes = true);

    /**
     * Construct a CSV file.
     * @param path               Path of the file.
     * @param sep                Separator.
     * @param quotes             Can the values be delimited by double quotes?
     */
    explicit RCSVFile(std::filesystem::path path, char sep = ',', bool quotes = true);

    /**
     * Destruct the file.
     */
    virtual ~RCSVFile() = default;

    /**
     * Get the path of the file.
     */
    [[nodiscard]] const std::filesystem::path& path() const { return path_; }

    /**
     * Open the file.
     * @exception RIOException   The file cannot be open.
     */
    void open();

    /**
     * Open a file.
     * @param path               Path of the file.
     * @param sep                Separator.
     * @param quotes             Can the values be delimited by double quotes?
     * @exception RIOException   The file cannot be open.
     */
    void open(const std::filesystem::path& path, char sep = ',', bool quotes = true);

    /**
     * Read the records from a string instead of a file.
     * @param text               Text.
     */
    void parse(const std::string& text);

    /**
     * Close the file.
     */
    void close();

    /**
     * Look if the file is open.
     */
    [[nodiscard]] bool isOpen() const { return static_cast<bool>(in_); }

    /**
     * Set the escape character (0 to disable it).
     * @param escape             Escape character.
     */
    void escape(const char escape) { escape_ = escape; }

    /**
     * Look if there is no more records to read.
     */
    [[nodiscard]] bool end() const;

    /**
     * Read the next record.
     * @exception RIOException   There is no more record, or the record is not valid.
     */
    void read();

    /**
     * Get the line number of the beginning of the last record read.
     */
    [[nodiscard]] std::size_t line() const { return recordLine_; }

    /**
     * Get the number of values of the last record read.
     */
    [[nodiscard]] std::size_t nbValues() const { return values_.size(); }

    /**
     * Look if the last record read has a value at a given index.
     * @param idx                Index.
     */
    [[nodiscard]] bool hasValue(const std::size_t idx) const { return idx < values_.size(); }

    /**
     * Get a value of the last record read.
     * @param idx                Index.
     * @exception RIOException   The record has no value at this index.
     */
    [[nodiscard]] const std::string& value(std::size_t idx) const;

    /**
     * Get a value of the last record read as a number.
     * @tparam T                 Type of the number.
     * @param idx                Index.
     * @param zero               Is an empty value considered as 0 (true) or as an error (false)?
     * @exception RIOException   The record has no value at this index, or the value is not a number of this type.
     */
    template<class T>
        [[nodiscard]] T value(const std::size_t idx, const bool zero = true) const
    {
        static_assert(std::is_arithmetic_v<T>, "Only numbers are supported");
        mLogModule("r.core");
        const auto& str = value(idx);
        if (str.empty() && zero)
            return T{};
        std::istringstream in(str);
        in.imbue(std::locale::classic());
        T number{};
        in >> number;
        if (in.fail() || in.peek() != std::istringstream::traits_type::eof() ||
            (std::is_unsigned_v<T> && str.front() == '-'))
            mThrowRIOException(nullptr, where(), ": column ", idx, ": '", str, "' is not a valid number");
        return number;
    }

    /**
     * Transform a string into a CSV value delimited by double quotes (the double quotes inside the string are doubled).
     * @param val                String.
     */
    [[nodiscard]] static std::string csvValue(std::string_view val);

    //-----------------------------------------------------
    RCSVFile(const RCSVFile&) = delete;
    RCSVFile& operator=(const RCSVFile&) = delete;

private:

    /**
     * Look if a character is a space to skip (a space or a tabulation that is not the separator).
     */
    [[nodiscard]] bool blank(const int c) const { return (c == ' ' || c == '\t') && c != asInt(sep_); }

    /**
     * Convert a character into the value returned by the streams (to compare it with get and peek).
     */
    [[nodiscard]] static int asInt(const char c) { return static_cast<unsigned char>(c); }

    /**
     * Read the next character (the line number is updated).
     */
    int get();

    /**
     * Skip an end of line ("\n", "\r\n" or "\r") if it is the next character.
     * @return true if an end of line was skipped.
     */
    bool skipEol();

    /**
     * Skip the blank characters and the empty lines.
     */
    void skipEmptyLines();

    /**
     * Start the reading.
     */
    void start();

    /**
     * Get the location of the current record for the error messages.
     */
    [[nodiscard]] std::string where() const;

    //-----------------------------------------------------
    std::filesystem::path path_;            ///< Path of the file.
    char sep_;                              ///< Separator.
    bool quotes_;                           ///< Can the values be delimited by double quotes?
    char escape_ = '\\';                    ///< Escape character (0 if none).
    std::unique_ptr<std::istream> in_;      ///< Stream read.
    std::size_t line_ = 1;                  ///< Current line.
    std::size_t recordLine_ = 0;            ///< Line of the last record read.
    std::vector<std::string> values_;       ///< Values of the last record read.
};


}  //-------- End of namespace evocomb ---------------------------------------------
