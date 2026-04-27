/*

	EvoComb

	RException.cpp

	Basic Exception - Implementation.

	Copyright 2007-2026 by Pascal Francq (pascal@francq.info).
	Copyright 2007-2008 by the Université Libre de Bruxelles (ULB).

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/



//------------------------------------------------------------------------------
// include files for EvoComb
#include "tools/r_exception.hpp"
using namespace evocomb;



//------------------------------------------------------------------------------
//
// class RException
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
RException::RException(std::string msg):
	msg_(std::move(msg))
{
}


//------------------------------------------------------------------------------
RException::RException(const std::string_view module, const std::string_view file, const int line, std::string msg):
	msg_(std::move(msg)), module_(module), line_(line)
{
	// Keep only the file name (__FILE__ may contain the full path).
	const auto pos = file.find_last_of("/\\");
	file_ = pos == std::string_view::npos ? file : file.substr(pos + 1);
}
