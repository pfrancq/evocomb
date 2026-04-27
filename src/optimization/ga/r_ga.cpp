/*

	EvoComb

	RGA.cpp

	Genetic Algorithm - Implementation.

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
// include files for ANSI C/C++
#include <iostream>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "ga/r_ga.hpp"

namespace evocomb {
	class RException;
}

using namespace evocomb;


//------------------------------------------------------------------------------
//
// class RGAException
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
RGAException::RGAException(const std::string& type,const std::string& str)
	: RException(type+": "+str), type_(type)
{
}


//------------------------------------------------------------------------------
RGAException::RGAException(const std::string_view module, const std::string_view srcFile, const int line, const std::string& str)
	: RException(module, srcFile, line, str)
{
}
