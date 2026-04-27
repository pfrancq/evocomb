/*

	EvoComb

	R2AdditiveChoquet.hpp

	The 2-additive Choquet Integral - Header.

	Copyright 2013-2026 by Pascal Francq (pascal@francq.info).

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
#include <string>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "matrix/r_symmetric_matrix.hpp"


//------------------------------------------------------------------------------
namespace evocomb{
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
 * The R2AdditiveChoquet class allows to compute the 2-additive Choquet integral for a given vector of criteria values.
 * It is an aggregating method that computes a global score from a set of criteria values by taking into account not
 * only the importance of each criterion, but also the importance of the interactions between pairs of criteria (for
 * example, a good student scores well in English and in mathematics).
 *
 * In practice, the models needs, for n criteria, the following parameters :
 * - n weights, \f$\phi(i)\f$ representing the importance of each criteria.
 * - \f$\frac{n(n-1)}{2}\f$ weights, \f$I_ij\f$ representing the importance of
 *   the interaction between a pair of criteria.
 *
 * The sum of these parameters must be 1.
 *
 * The 2-additive Choquet integral for a given object, \f$f\f$ can then be computed using [1]:
 * \f[
 		C_{\mu}(f)=
                  \sum_{i,j|I_{i,j}>0}(f(i)\wedge f(j))I_{ij}
                 +\sum_{i,j|I_{i,j}\leq 0}(f(i)\vee f(j))\left|I_{ij}\right|
                 +\sum_{i\in N}f(i)\left[\phi(i)-\frac{1}{2}\sum_{j\neq i}\left|I_{ij}\right|\right]
 * \f] where \f$f(i)\f$ is the value of the object for the \f$i\f$th criteria, \f$\wedge\f$ is the minimum operator,
 * and \f$\vee\f$ is the maximum operator.
 *
 * <div class="groupHeader">
 * References
 * </div>
 * [1] M. Grabisch (2006), L'utilisation de l'intégrale de Choquet en aide multicritère à la décision. <em>Newsletter of
 * the European Working
 *   Group "Multicriteria Aid for Decisions"</em>, 3(14), pp. 5-10.
 * @short 2-Additive Choquet Integral.
 */
class R2AdditiveChoquet
{
public:

	/**
	 * Constructor.
	 * @param n               Number of criteria.
	 * @param name            Name of the function computed.
	*/
	explicit R2AdditiveChoquet(size_t n, std::string name = {});

	/**
	 * Constructor.
	 * @param n               Number of criteria.
	 * @param params          Parameters. params(i,i) is supposed to be the weights of each criteria while params(i,j)
	 *                        is supposed to be the weights of each interactions.
	 * @param name            Name of the function computed. \exception std::range_error is generated if the container
	 * has not a value for each criteria. \exception std::invalid_argument is generated if the sum of parameters is not
	 * equal to 1.
	*/
	R2AdditiveChoquet(size_t n, RSymmetricMatrix& params, std::string name={});

	/**
    * @return the name of the function computed.
    */
	[[nodiscard]] std::string name() const { return name_; }

	/**
	 * Verify that the sum of the parameters is one.
	 * @return true if it is the case.
    */
	bool verify();

	/**
	 * Set the parameters of the Choquet integral.
	 * @param params          Parameters. params(i,i) is supposed to be the weights of each criteria while params(i,j)
	 * is supposed to be the weights of each interactions. \exception std::range_error is generated if the container has
	 * not a value for each criteria. \exception std::invalid_argument is generated if the sum of parameters is not
	 * equal to 1.
	 */
	void setParams(RSymmetricMatrix& params);

	/**
	 * Set the weight of a given criteria.
	 * @param idx             Index of the criteria.
	 * @param weight          Weight of the criteria.
    */
	void setCriteria(size_t idx, double weight);

	/**
	 * Get the weight of a given criteria.
    * @param idx				Index of the criteria.
    * @return the weight of the criteria.
    */
	[[nodiscard]] double criteria(size_t idx) const;

	/**
	 * Set the weight of the interaction of a pair of criteria.
	 * @param idx1            Index of the first criteria.
	 * @param idx2            Index of the second criteria.
	 * @param weight          Weight of the interaction.
    */
	void setInteraction(size_t idx1, size_t idx2, double weight);

	/**
	 * Get the weight of the interaction of a pair of criteria.
	 * @param idx1            Index of the first criteria.
	 * @param idx2            Index of the second criteria.
    * @return the weight of the interaction.
    */
	[[nodiscard]] double interaction(size_t idx1,size_t idx2) const;

	/**
	 * Compute the 2-Additive Choquet integral of an object defined by a set of criteria stored in a container.
     * @param values             Values of the criteria for the objects.
     * @return the 2-Additive Choquet integral.
	 * \exception std::range_error is generated if the container has not a value for each criteria. \exception
	 * std::invalid_argument is generated if the sum of parameters is not equal to 1.
    */
	double compute(const std::vector<double>& values);

	/**
	 * Destructor.
    */
	virtual ~R2AdditiveChoquet() = default;

private:

	//-----------------------------------------------------
	std::string name_;				///< Name representing the function computed.
	size_t nbCriteria_;				///< Number of criteria.
	RSymmetricMatrix params_;		///< Parameters. Params(i,i) stores the weights of each criteria while Params(i,j) stores the weights of each interaction.
	bool verified_;					///< Parameters verified?
};

}  //-------- End of namespace evocomb -----------------------------------------------
