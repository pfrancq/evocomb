/*

	EvoComb

	RPromSol.cpp

	PROMETHEE Solutions - Implementation.

	Copyright 2000-2026 by Pascal Francq (pascal@francq.info).
	Copyright 2000-2008 by the Université Libre de Bruxelles (ULB).

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
#include <utility>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "multicriteria/r_prom_sol.hpp"
using namespace evocomb;



//------------------------------------------------------------------------------
//
// class RPromSol
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
RPromSol::RPromSol(std::string name):
	name_(std::move(name))
{
}
