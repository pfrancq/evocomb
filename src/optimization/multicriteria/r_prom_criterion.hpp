/*

	EvoComb

	RPromCriterion.hpp

	PROMETHEE Criterion - Header.

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
#include <cstdint>
#include <memory>
#include <string>
#include <vector>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "multicriteria/r_prom_crit_value.hpp"


//------------------------------------------------------------------------------
namespace evocomb{
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// forward declaration
class RPromKernel;

//------------------------------------------------------------------------------
/**
* Different types of criteria.
* @short Criteria Type.
*/
enum class RCriteriaType
{
	Minimize                 /** The criteria must be maximized.*/,
	Maximize                 /** The criteria must be minimized.*/
};


//------------------------------------------------------------------------------
/**
* The RPromCriterion class provides a representation for a Promethee Criterion.
* @short PROMETHEE Criterion.
*/
class RPromCriterion
{
public:

	/**
	 * The RPromCriterion provides a smart pointer on a RPromCriterion instance.
	 */
	using Ptr = std::unique_ptr<RPromCriterion>;

	/**
	* Construct a criterion.
	* @param type           Type of the criterion.
	* @param weight         Weight of the criterion.
	* @param name           Name of the criterion.
	*/
	RPromCriterion(RCriteriaType type, double weight, std::string name={});

	/**
    * @return if the criteria is active.
    */
	[[nodiscard]] bool isActive() const { return isActive_; }

	/**
	 * Get the type of criteria.
	 */
	[[nodiscard]] RCriteriaType type() const { return type_; }

	/**
	* Get the weight.
	*/
	[[nodiscard]] double weight() const { return weight_; }

	/**
	 * Get the name of the criterion.
	 */
	[[nodiscard]] std::string name() const { return name_; }

	/**
	* Compute the preference of a solution with the value u and a solution with the value v. This preference must be in
	* [0,1].
	*/
	virtual double computePref(double u,double v)=0;

	/**
	* Calculate the Fi Crit for the different solutions.
	*/
	void computeFiCrit();

	/**
	 * Destructor.
	 */
	virtual ~RPromCriterion() = default;

protected:

	//-----------------------------------------------------
	uint64_t index_;						///< The index of the criterion.
	std::string name_;						///< Name of the criterion.
	RCriteriaType type_;					///< The type of criteria.
	double weight_;							///< Weight of this criterion.
	bool isActive_;							///< Is the criteria active ?
	std::vector<RPromCritValue> values_;	///< Values.

	//-----------------------------------------------------
	friend class RPromKernel;
};


//------------------------------------------------------------------------------
/**
* The RPromLinearCriterion class provides a representation for a PROMETHEE Criterion.
* @short Linear PROMETHEE Criterion.
*/
class RPromLinearCriterion : public RPromCriterion
{
public:

	/**
	* Construct a criterion.
	* @param type			Type of the criterion.
	* @param p              Preference threshold.
	* @param q              Indifference threshold.
	* @param weight         Weight of the criterion.
	* @param name           Name of the criterion.
	*/
	RPromLinearCriterion(RCriteriaType type, double p, double q, double weight, std::string name={});

	/**
	* @return the preference threshold.
	*/
	[[nodiscard]] double p() const { return p_; }

	/**
	* @return the indifference threshold.
	*/
	[[nodiscard]] double q() const { return q_; }

	/**
	* Compute the preference of a solution with the value u and a solution with the value v. This preference must be in
	* [0,1].
	*
	* This method compares the normalized difference of values, d:
	* -# If d<Q=: No solutions must be preferred to the other one. The method
	*            returns 0.
	* -# If Q<d<P: One of the solution has a degree of preference computed as
	*              (d-Q)/(P-Q).
	* -# If d>=P: One of the solution must always be preferred. The method
	*            returns 1.
	* @param u              Value of the first solution.
	* @param v              Value of the second solution.
	* @return the result of the preference function defined.
	*/
	double computePref(double u, double v) override;

protected:

	//-----------------------------------------------------
	double p_ = 0.0;						///< Value for the preference threshold of the PROMETHEE method.
	double q_ = 0.0;						///< Value for the indifference threshold of the PROMETHEE method.
};


}  //-------- End of namespace evocomb ---------------------------------------------
