/*

	EvoComb

	RCSVFile.cpp

	CSV File - Implementation.

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
#include <fstream>
#include <utility>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "io/r_csv_file.hpp"
using namespace evocomb;


//------------------------------------------------------------------------------
mLogModule("r.core");


//------------------------------------------------------------------------------
/**
 * End of the stream.
 */
static constexpr auto Eof = std::istream::traits_type::eof();



//------------------------------------------------------------------------------
//
// class RCSVFile
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
RCSVFile::RCSVFile(const char sep, const bool quotes) :
    sep_(sep), quotes_(quotes)
{
}


//------------------------------------------------------------------------------
RCSVFile::RCSVFile(std::filesystem::path path, const char sep, const bool quotes) :
    path_(std::move(path)), sep_(sep), quotes_(quotes)
{
}


//------------------------------------------------------------------------------
void RCSVFile::open()
{
    close();
    auto file = std::make_unique<std::ifstream>(path_, std::ios::binary);
    if (!file->is_open())
        mThrowRIOException(nullptr, path_.string(), ": cannot open the file");
    in_ = std::move(file);
    start();
}


//------------------------------------------------------------------------------
void RCSVFile::open(const std::filesystem::path& path, const char sep, const bool quotes)
{
    path_ = path;
    sep_ = sep;
    quotes_ = quotes;
    open();
}


//------------------------------------------------------------------------------
void RCSVFile::parse(const std::string& text)
{
    close();
    path_.clear();
    in_ = std::make_unique<std::istringstream>(std::move(text));
    start();
}


//------------------------------------------------------------------------------
void RCSVFile::close()
{
    in_.reset();
    values_.clear();
    line_ = 1;
    recordLine_ = 0;
}


//------------------------------------------------------------------------------
void RCSVFile::start()
{
    // Skip a UTF-8 byte order mark
    char bom[3] = {};
    in_->read(bom, 3);
    if (in_->gcount() != 3 || bom[0] != '\xEF' || bom[1] != '\xBB' || bom[2] != '\xBF')
    {
        in_->clear();
        in_->seekg(0);
    }
    skipEmptyLines();
}


//------------------------------------------------------------------------------
int RCSVFile::get()
{
    const auto c = in_->get();
    if (c == '\n' || (c == '\r' && in_->peek() != '\n'))
        line_++;
    return c;
}


//------------------------------------------------------------------------------
bool RCSVFile::skipEol()
{
    const auto c = in_->peek();
    if (c != '\n' && c != '\r')
        return false;
    get();
    if (c == '\r' && in_->peek() == '\n')
        get();
    return true;
}


//------------------------------------------------------------------------------
void RCSVFile::skipEmptyLines()
{
    do
    {
        while (blank(in_->peek()))
            get();
    }
    while (skipEol());
}


//------------------------------------------------------------------------------
bool RCSVFile::end() const
{
    return !in_ || in_->peek() == Eof;
}


//------------------------------------------------------------------------------
std::string RCSVFile::where() const
{
    const auto name = path_.empty() ? std::string("CSV") : path_.string();
    return name + " (" + std::to_string(recordLine_) + ")";
}


//------------------------------------------------------------------------------
void RCSVFile::read()
{
    if (!in_)
        mThrowRIOException(nullptr, "The CSV file is not open");
    values_.clear();
    recordLine_ = line_;
    if (end())
        mThrowRIOException(nullptr, where(), ": end of file reached");

    while (true)
    {
        std::string value;
        while (blank(in_->peek()))
            get();

        if (quotes_ && in_->peek() == '"')
        {
            // Value delimited by double quotes
            get();
            while (true)
            {
                auto c = get();
                if (c == Eof)
                    mThrowRIOException(nullptr, where(), ": a value does not finish with a quote");
                if (escape_ && c == asInt(escape_))
                {
                    c = get();
                    if (c == Eof)
                        mThrowRIOException(nullptr, where(), ": a value does not finish with a quote");
                }
                else if (c == '"')
                {
                    if (in_->peek() != '"')
                        break;
                    get();
                }
                value += static_cast<char>(c);
            }

            // Only spaces are allowed until the separator or the end of the line
            while (blank(in_->peek()))
                get();
            if (const auto c = in_->peek(); c != asInt(sep_) && c != '\n' && c != '\r' && c != Eof)
                mThrowRIOException(nullptr, where(), ": invalid character '", static_cast<char>(c), "' after a quote");
        }
        else
        {
            // Value delimited by the separator or the end of the line
            for (auto c = in_->peek(); c != asInt(sep_) && c != '\n' && c != '\r' && c != Eof; c = in_->peek())
            {
                get();
                if (escape_ && c == asInt(escape_) && in_->peek() != Eof)
                    c = get();
                value += static_cast<char>(c);
            }
            while (!value.empty() && (value.back() == ' ' || value.back() == '\t'))
                value.pop_back();
        }
        values_.push_back(std::move(value));

        // Next value, or end of the record
        if (in_->peek() == asInt(sep_))
        {
            get();
            continue;
        }
        skipEol();
        break;
    }
    skipEmptyLines();
}


//------------------------------------------------------------------------------
const std::string& RCSVFile::value(const std::size_t idx) const
{
    if (idx >= values_.size())
        mThrowRIOException(nullptr, where(), ": the record has not ", idx + 1, " values");
    return values_[idx];
}


//------------------------------------------------------------------------------
std::string RCSVFile::csvValue(const std::string_view val)
{
    std::string res;
    res.reserve(val.size() + 2);
    res += '"';
    for (const auto c : val)
    {
        if (c == '"')
            res += '"';
        res += c;
    }
    res += '"';
    return res;
}
