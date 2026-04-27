/*

	EvoComb

	RPromSol.hpp

	PROMETHEE Solutions - Header.

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
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>


//------------------------------------------------------------------------------
namespace evocomb{
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
* The RPromSol class provides a representation of a solution in the PROMETHEE method.
* @short PROMETHEE Solution.
*/
class RPromSol
{
public:

	/**
	 * The RPromSol::Ptr provides a smart pointer to a RPromSol instance.
	 */
	using Ptr = std::unique_ptr<RPromSol>;

	/**
	* Construct a PROMETHEE solution.
	* @param name				Name of the solution
	*/
	explicit RPromSol(std::string name);

	/**
	* @return Fi of the solution.
	*/
	[[nodiscard]] double fi() const { return fi_; }

	/**
	* @return Fi+ of the solution.
	*/
	[[nodiscard]] double fiPlus() const { return fiPlus_; }

	/**
	* @return Fi- of the solution.
	*/
	[[nodiscard]] double fiMinus() const { return fiMinus_; }

	/**
	* @return Index of the solution.
	*/
	[[nodiscard]] size_t index() const { return index_; }

	/**
	* Return the name of the solution.
	*/
	[[nodiscard]] std::string name() const { return name_; }

	/**
	* Destruct a PROMETHEE solution.
	*/
	virtual ~RPromSol() = default;

private:

	//-----------------------------------------------------
	uint64_t index_ = -1;			///< The index of the solution.
	std::string name_;				///< Name of the solution.
	double fi_ = 0.0;				///< The fi of the solution.
	double fiPlus_ = 0.0;			///< The fi+ of the solution.
	double fiMinus_ = 0.0;			///< The fi- of the solution.

	//-----------------------------------------------------
	friend class RPromKernel;
	friend class RPromCriterion;
};


}  //-------- End of namespace evocomb ---------------------------------------------
