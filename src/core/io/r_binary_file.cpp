/*

	EvoComb

	RBinaryFile.cpp

	Binary file - Implementation.

	Copyright 2006-2026 by Pascal Francq (pascal@francq.info).
	Copyright 2006-2008 by the Université Libre de Bruxelles (ULB).

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
// include files for EvoComb
#include "io/r_binary_file.hpp"
#include "app/r_logger.hpp"
using namespace evocomb;


//------------------------------------------------------------------------------
mLogModule("r.core");



//------------------------------------------------------------------------------
//
// class RBinaryFile
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
void RBinaryFile::readBytes(void* buffer, const std::size_t nb)
{
    if (nb == 0)
        return;
    if (static_cast<std::streamoff>(nb) > size() - pos())
        mThrowRIOException(this, "Cannot read ", nb, " bytes: only ", size() - pos(), " bytes left");
    read(static_cast<char*>(buffer), nb);
}


//------------------------------------------------------------------------------
RBinaryFile& RBinaryFile::operator>>(std::string& str)
{
    std::uint64_t len = 0;
    readBytes(&len, sizeof(len));
    if (len > static_cast<std::uint64_t>(size() - pos()))
        mThrowRIOException(this, "Invalid length of string (", len, ")");
    str.resize(len);
    readBytes(str.data(), str.size());
    return *this;
}


//------------------------------------------------------------------------------
RBinaryFile& RBinaryFile::operator<<(const std::string_view str)
{
    const auto len = static_cast<std::uint64_t>(str.size());
    write(reinterpret_cast<const char*>(&len), sizeof(len));
    write(str.data(), str.size());
    return *this;
}
