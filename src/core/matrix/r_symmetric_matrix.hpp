/*

	EvoComb

	RSymmetricMatrix.hpp

	Matrix class - Header.

	Copyright 1999-2026 by Pascal Francq (pascal@francq.info).
	Copyright 1999-2008 by the Université Libre de Bruxelles (ULB).

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
// include files for EvoComb
#include "matrix/r_lower_triangular_matrix.hpp"


//------------------------------------------------------------------------------
namespace evocomb{
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
* The RSymmetricMatrix class provides a representation of a symmetric matrix.
* @short Symmetric Matrix.
*/
class RSymmetricMatrix : public RLowerTriangularMatrix
{
public:

	/**
	* Construct a square symmetric matrix of a given size.
	* @param size            Size of the matrix.
	*/
	explicit RSymmetricMatrix(size_t size);

	/**
	 * Construct a matrix.
	 * @param matrix		Matrix.
	 */
	explicit RSymmetricMatrix(const std::vector<std::vector<double>>& matrix);

	/**
	 * Get the type of the matrix.
	 */
	[[nodiscard]] RMatrixType type() const override { return RMatrixType::Symmetric; }

	/**
	* Return a specific element of the matrix (const version).
	* @param i               Line number of the element.
	* @param j               Column number of the element.
	*/
 	double operator()(size_t i,size_t j) const override;

	/**
	* Return a specific element of the matrix (const version).
	* @param i               Line number of the element.
	* @param j               Column number of the element.
	*/
 	double& operator()(size_t i,size_t j) override;

	/**
	* Destruct the matrix.
	*/
	~RSymmetricMatrix() override = default;
};


}  //-------- End of namespace evocomb ---------------------------------------------
