/*

	EvoComb

	RIO.cpp

	Generic Class - Implementation.

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
// include files for ANSI C/C++
#include <filesystem>
#include <system_error>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "io/r_io.hpp"
#include "io/file_tools.hpp"
#include "io/r_io_file.hpp"
#include "io/r_text_parser.hpp"
using namespace evocomb;



//------------------------------------------------------------------------------
//
// class RSmartTempFile
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
RSmartTempFile::RSmartTempFile(const bool remove):
	path_(FileTools::tempFile()),
	remove_(remove)
{
}


//------------------------------------------------------------------------------
void RSmartTempFile::remove(const bool remove)
{
	remove_ = remove;
}


//------------------------------------------------------------------------------
RSmartTempFile::~RSmartTempFile()
{
	// The error_code overload never throws (a destructor must not throw), and an error is ignored
	if (remove_)
	{
		std::error_code ec;
		std::filesystem::remove(path_, ec);
	}
}



//------------------------------------------------------------------------------
//
// class RIOException
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
RIOException::RIOException(const std::string& str)
	: RException(str)
{
}


//------------------------------------------------------------------------------
std::string RIOException::detail(const RIOFile* file, const std::string& str)
{
	if(!file)
		return str;
	return file->path().string() + " (" + std::to_string(file->pos()) + "): " + str;
}


//------------------------------------------------------------------------------
std::string RIOException::detail(const RTextParser* file, const std::string& str)
{
	if(!file)
		return str;
	const std::string pos = std::to_string(file->line()) + ":" + std::to_string(file->col());
	if(const auto path = file->path().string(); !path.empty())
		return path + " (" + pos + "): " + str;
	return "(" + pos + "): " + str;
}


//------------------------------------------------------------------------------
RIOException::RIOException(const RIOFile* file, const std::string_view module, const std::string_view srcFile, const int line, const std::string& str)
	: RException(module, srcFile, line, detail(file, str))
{
}


//------------------------------------------------------------------------------
RIOException::RIOException(const RTextParser* file, const std::string_view module, const std::string_view srcFile, const int line, const std::string& str)
	: RException(module, srcFile, line, detail(file, str))
{
}


//------------------------------------------------------------------------------
RIOException::RIOException(std::nullptr_t, const std::string_view module, const std::string_view srcFile, const int line, const std::string& str)
	: RException(module, srcFile, line, str)
{
}
