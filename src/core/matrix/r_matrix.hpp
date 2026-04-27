/*

	EvoComb

	RMatrix.hpp

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
#include "matrix/r_generic_matrix.hpp"


//------------------------------------------------------------------------------
namespace evocomb{
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
* The RMatrix class provides a representation of a matrix as a given number of vector, each vector representing a line
*
* Here are some examples:
* @code
* RMatrix a({{1, 2}, {2, 2}});
* RMatrix b(2,2);
* RMatrix c;
*
* b(0,0) = 3; b(0,1)=2;
* b(1,1) = 1; b(1,1)=4;
* c = a + b;
* c = a - b;
* c = a * b;
* c = (2 * a) + (b * a);
*
* RCursor<RVector> Lines(c.GetLines());
* for(Lines.Start();!Lines.End();Lines.Next())
* {
* 	RNumCursor<double> Cols(Lines()->GetCols());
* 	for(Cols.Start();!Cols.End();Cols.Next())
* 		std::cout<<Cols()<<"\t";
* 	std::cout<<std::endl;
* }
* @endcode
* @short Matrix.
*/
class RMatrix : public RGenericMatrix
{
public:

	/**
	* Construct a square matrix of a given size.
	* @param size            Size of the matrix.
	*/
	explicit RMatrix(size_t size);

	/**
	* Construct a matrix (L,C).
	* @param lines           Initial number of lines.
	* @param cols            Initial number of columns.
	*/
	RMatrix(size_t lines, size_t cols);

protected:

	/**
	* Protected constructor a matrix (L,C) for which nothing is preallocated.
	* @param lines           Initial number of lines.
	* @param cols            Initial number of columns.
	*/
	RMatrix(size_t lines, size_t cols, bool);

public:

	/**
	 * Construct a matrix.
	 * @param matrix		Matrix.
	 */
	explicit RMatrix(const std::vector<std::vector<double>>& matrix);

	/**
	* Construct a matrix from another one.
	* @param matrix          Matrix.
	*/
	RMatrix(const RMatrix& matrix) = default;

	/**
	* Destruct the matrix.
	*/
	~RMatrix() override = default;

	/**
	 * Initialize the matrix with a given value.
	 * @param val            Value to assign.
	 */
	void init(double val) override;

	/**
	* Verify if the matrix has a given size, and increase them if necessary.
	* @param newLines        New line number.
	* @param newCols         New column number.
	* @param fill            Elements must be filled with a value.
	* @param val             Value used eventually to fill the elements created.
	*/
	void verifySize(size_t newLines, size_t newCols, bool fill, double val) override;

	/**
	 * Get the type of the matrix.
	 */
	[[nodiscard]] RMatrixType type() const override { return RMatrixType::Normal; }

	/**
	* Make the matrix symmetric by copying the "left-upper" part in the "right-bottom" part.
	*/
	void symmetrize();

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
	RMatrix& operator=(const RMatrix& matrix) = default;

	/**
	* Add a matrix to the current one.
	* @param matrix          Matrix to add.
	*/
	RMatrix& operator+=(const RMatrix& matrix);

	/**
	* Subtract a matrix from the current one.
	* @param matrix          Matrix to subtract.
	*/
  	RMatrix& operator-=(const RMatrix& matrix);

	/**
	* Multiply a matrix with a given number.
	* @param arg             Number.
	*/
	RMatrix& operator*=(double arg);

	/**
	* Divide a matrix with a given number.
	* @param arg             Number.
	*/
	RMatrix& operator/=(double arg);

	/**
	* Multiply a matrix with the current one. It is important to remember that the matrix multiplication is not
	* commutative. So the next code defines two different matrices TempA and TempB.
	* @code
	* void Test(RMatrix &A,RMatrix &B)
	* {
	* 	RMatrix TempA(A);
	* 	RMatrix TempB(B);
	*
	* 	TempA *= B;
	* 	TempB *= A;
	* }
	* @endcode
	* @param matrix          Matrix to multiply.
	*/
	RMatrix& operator*=(const RMatrix& matrix);

protected:

	//-----------------------------------------------------
	std::vector<RVector> lines_;				///< Lines
};


//------------------------------------------------------------------------------
/**
* Addition of two matrices.
* @param arg1                Matrix.
* @param arg2                Matrix.
*/
RMatrix operator+(const RMatrix& arg1, const RMatrix& arg2);


//------------------------------------------------------------------------------
/**
* Subtraction of two matrices.
* @param arg1                Matrix.
* @param arg2                Matrix.
*/
RMatrix operator-(const RMatrix& arg1, const RMatrix& arg2);


//------------------------------------------------------------------------------
/**
* Multiplication of a matrix and a number.
* @param arg1                Matrix.
* @param arg2                Number.
*/
RMatrix operator*(const RMatrix& arg1, double arg2);


//------------------------------------------------------------------------------
/**
* Multiplication of a matrix and a number.
* @param arg1                Number.
* @param arg2                Matrix.
*/
RMatrix operator*(double arg1, const RMatrix& arg2);


//------------------------------------------------------------------------------
/**
* Division of a matrix and a number.
* @param arg1                Matrix.
* @param arg2                Number.
*/
RMatrix operator/(const RMatrix& arg1, double arg2);


//------------------------------------------------------------------------------
/**
* Division of a matrix and a number.
* @param arg1                Number.
* @param arg2                Matrix.
*/
RMatrix operator/(double arg1, const RMatrix& arg2);


//------------------------------------------------------------------------------
/**
* Multiplication of two matrixes.
* @param arg1                Matrix.
* @param arg2                Matrix.
*/
RMatrix operator*(const RMatrix& arg1,const RMatrix& arg2);


}  //-------- End of namespace evocomb ---------------------------------------------
