/*

	EvoComb

	RPromKernel.hpp

	PROMETHEE Kernel - Header.

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
#include <string>
#include <vector>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "multicriteria/r_prom_sol.hpp"
#include "multicriteria/r_prom_criterion.hpp"
#include "app/r_logger.hpp"
#include "tools/r_json.hpp"


//------------------------------------------------------------------------------
namespace evocomb{
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
* The RPromKernel provides a kernel to manage PROMETHEE session.
* @code
* // Create kernel and the criteria
* RPromKernel kernel("Test");
* kernel.addCriterion(std::make_unique<RPromLinearCriterion>(RPromCriterion::Minimize, 0.2, 0.05, 1.0, "Price"));
* kernel.addCriterion(std::make_unique<RPromLinearCriterion>(RPromCriterion::Minimize, 0.2, 0.05, 1.0, "Cons"));
* kernel.addCriterion(std::make_unique<RPromLinearCriterion>(RPromCriterion::Maximize, 0.2, 0.05, 1.0, "Comfort"));
* kernel.addCriterion(std::make_unique<RPromLinearCriterion>(RPromCriterion::Maximize, 0.2, 0.05, 1.0, "Power"));
*
* // Create solutions and assign values
* kernel.createSolutions({"car1", "car2", "car3", "car4"});
* kernel.assign(0, {8.75, 6.2, 1, 30});
* kernel.assign(1, {13.75, 7.5, 1, 50});
* kernel.assign(2, {25, 8, 3, 80});
* kernel.assign(3, {62.5, 20, 2, 120});
*
* // Compute Promethee
* kernel.ComputePrometheeII();
*
* // Print best soution
* std::cout << "Best solution " << kernel->bestSolution().index();
* @endcode
*
* The criteria can also be defined with a JSON array (see addCriteria):
* @code
* kernel.addCriteria(RJSON::parse(R"([
*     {"name": "Price", "type": "minimize", "p": 0.2, "q": 0.05, "weight": 1.0},
*     {"name": "Comfort", "type": "maximize"}
* ])"));
* @endcode
* @short PROMETHEE Kernel.
*/
class RPromKernel
{
public:

	/**
	* Construct the kernel.
	* @param logger				Logger where to log.
	* @param name				The name of the kernel.
	*/
	explicit RPromKernel(RLogger& logger, std::string name);

	/**
	* Calculate PROMETHEE II.
	*/
	virtual void computePrometheeII();

	/**
	 * Add a new criterion.
	 * @param crit           Pointer to the new criterion (will be moved).
	 */
	void addCriterion(RPromCriterion::Ptr&& crit);

	/**
	 * Add (linear) criteria defined by a JSON array. Each element is an object:
	 * @code
	 * {
	 *   "name": "Price",           // Name (optional).
	 *   "type": "minimize",        // "minimize" or "maximize" (mandatory).
	 *   "p": 0.2,                  // Preference threshold (default: 0.2).
	 *   "q": 0.05,                 // Indifference threshold (default: 0.05).
	 *   "weight": 1.0              // Weight (default: 1.0).
	 * }
	 * @endcode
	 * The criteria are only added if they are all valid.
	 * @param criteria           JSON array.
	 * @exception RException     The description is invalid.
	 */
	void addCriteria(const RJSON& criteria);

	/**
	 * Get the number of criteria.
	 */
	[[nodiscard]] size_t nbCriteria() const { return criteria_.size(); }

	/**
	 * @return a pointer to a given criterion.
	 * @param name           Name of the criterion.
	 */
	[[nodiscard]] RPromCriterion& criterion(const std::string& name) const;

	/**
	 * @return a pointer to a given criterion.
	 * @param id             Identifier of the criterion.
	 */
	[[nodiscard]] RPromCriterion& criterion(uint64_t id) const;

	/**
	 * Create a set of solutions.
	 * @param names			Names of the different solutions.
	 */
	virtual void createSolutions(const std::vector<std::string>& names);

	/**
	* Add a new solution.
	* @param solution           Solution to add (will be moved).
	*/
	void addSolution(RPromSol::Ptr&& solution);

	/**
	 * @return the number of solutions managed by the kernel.
	 */
	[[nodiscard]] size_t nbSolutions() const { return solutions_.size(); }

	/**
	* Assign a value to a criterion.
	* @param solIdx			Solution index.
	* @param critIdx		Criteria index.
	* @param v				Value.
	*/
	virtual void assign(uint64_t solIdx, uint64_t critIdx, double v);

	/**
	 * Set the values for a solution.
	* @param solIdx			Solution index.
	 * @param values		Values.
	 */
	virtual void assign(uint64_t solIdx, const std::vector<double>& values);

	/**
	* @return the best solution.
	*/
	const RPromSol& bestSolution() const;

	/**
	* @return the solution vector ordered descending by their fitness.
	*/
	const std::vector<RPromSol*>& solutions() const;

	/**
    * @return the Fi of the worst solution.
    */
	double minFi() const;

	/**
    * @return the Fi of the best solution.
    */
	double maxFi() const;

	/**
	* Clear the kernel.
	*/
	void clear();

	/**
	* Clear the solutions of the kernel.
	*/
	void clearSols();

	/**
	 * Print the solutions ordered on screen.
    */
	void print() const;

	/**
	* Destruct the kernel.
	*/
	virtual ~RPromKernel() = default;

private:

	/**
	 * Order the solutions by descending order of their fitness.
	 */
	void orderSolutions() const;

	//-----------------------------------------------------
	RLogger& logger_;									///< Logger.
	std::string name_;									///< The name of the kernel.
	std::vector<RPromSol::Ptr> solutions_;				///< Solutions.
	mutable std::vector<RPromSol*> orderedSolutions_;	///< Solutions ordered by fitness.
	std::vector<RPromCriterion::Ptr> criteria_;			///< Criteria
	mutable bool mustReOrder_ = false;					///< Look if the solution must be reordered (for example after a computing).

	//-----------------------------------------------------
	friend class RPromCriterion;
};


}  //-------- End of namespace evocomb ---------------------------------------------
