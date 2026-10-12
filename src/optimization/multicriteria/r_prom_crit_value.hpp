/*

	EvoComb

	RPromCritValue.hpp

	PROMETHEE Criterion Value - Header.

	Copyright 2000-2026 by Pascal Francq (pascal@francq.info).
	Copyright 2000 by Thomas L'Eglise.
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
#pragma once


//------------------------------------------------------------------------------
// include files for ANSI C/C++
#include <memory>


//------------------------------------------------------------------------------
namespace evocomb{
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
* The RPromCritValue class provides a representation for a value of a solution for a given criterion.
* @short PROMETHEE Solution value.
*/
class RPromCritValue
{
public:

	/**
	 * The RPromCritValue::Ptr provides a smart pointer on a RPromCritValue instance.
	 */
	using Ptr = std::unique_ptr<RPromCritValue>;

	/**
	* Construct a value for a solution.
	*/
	RPromCritValue() = default;

	/**
	* Construct a value for a solution.
	* @param value          The value for the solution/criterion.
	*/
	explicit RPromCritValue(double value);

private:

	//-----------------------------------------------------
	double fi_ = 0.0;				///< The Fi crit used by PROMETHEE.
	double fiPlus_ = 0.0;			///< The Fi+ crit used by PROMETHEE.
	double fiMinus_ = 0.0;			///< The Fi- crit used by PROMETHEE.
	double value_ = 0.0;			///< The value of the solution/criterion.

	//-----------------------------------------------------
	friend class RPromKernel;
	friend class RPromCriterion;
};


}  //-------- End of namespace evocomb ---------------------------------------------
