/*

	EvoComb

	RLowerTriangularMatrix.hpp

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
#include "matrix/r_vector.hpp"
#include "matrix/r_matrix.hpp"
#include "matrix/r_generic_matrix.hpp"


//------------------------------------------------------------------------------
namespace evocomb{
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
* The RLowerTriangularMatrix class provides a representation of a lower triangular matrix.
* @code
* const RLowerTriangular a({1.0}, {2, 3});   // Square matrix of (2,2)
* std::cout << a(0,1) << std::endl;  // OK, print 0 (use the const (i,j) operator).
* a(0,1)=45.0;						 // !!! Generate an exception since Matrix(i,j)=0 ∀ j>i
* a(0,1)=0.0;						 // !!! Generate an exception : In fact, Matrix(i,j) cannot be written ∀ j>i
* if (std::as_const(a)(0,1) == 0.0)
*	std::cout << "This work since using std::as_const ensure that we don't try to write" << std::endl;
* @endcode
* @short Lower Triangular Matrix.
*/
class RLowerTriangularMatrix : public RMatrix
{
public:

	/**
	* Construct a square matrix of a given size.
	* @param size            Size of the matrix.
	*/
	explicit RLowerTriangularMatrix(size_t size);

	/**
	* Construct a matrix from another one.
	* @param matrix          Matrix.
	*/
	RLowerTriangularMatrix(const RLowerTriangularMatrix& matrix) = default;

	/**
	 * Construct a matrix.
	 * @param matrix		Matrix.
	 */
	explicit RLowerTriangularMatrix(const std::vector<std::vector<double>>& matrix);

	/**
	* Destruct the matrix.
	*/
	~RLowerTriangularMatrix() override = default;

	/**
	* Verify if the matrix has a given size, and increase them if necessary.
	* @param newlines        New line number.
	* @param newCols         New column number.
	* @param fill            Elements must be filled with a value.
	* @param val             Value used eventually to fill the elements created.
	*/
	void verifySize(size_t newlines, size_t newCols, bool fill, double val) override;

	/**
	 * Get the type of the matrix.
	 */
	[[nodiscard]] RMatrixType type() const override { return RMatrixType::LowerTriangular; }

	/**
	* Return a specific element of the matrix (const version).
	* @param i               Line number of the element.
	* @param j               Column number of the element.
	*/
 	double operator()(size_t i, size_t j) const override;

	/**
	* Return a specific element of the matrix (const version).
	* @param i               Line number of the element.
	* @param j               Column number of the element.
	*/
 	double& operator()(size_t i, size_t j) override;

	/**
	* Assign operator.
	* @param matrix          Matrix.
	*/
	RLowerTriangularMatrix& operator=(const RLowerTriangularMatrix& matrix) = default;
};


}  //-------- End of namespace evocomb ---------------------------------------------
