/*

	EvoComb

	RSparseSymmetricMatrix.hpp

	Sparse Symmetric Matrix - Header.

	Copyright 2008-2026 by Pascal Francq (pascal@francq.info).

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
#include "matrix/r_sparse_matrix.hpp"


//------------------------------------------------------------------------------
namespace evocomb{
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
 * The RSparseSymmetricMatrix class provides a square sparse matrix where element (i,j) equals element (j,i). Only the
 * lower part is stored: line i contains the elements (i,j) with j ≤ i.
 * @code
 * evocomb::RSparseSymmetricMatrix a(4);
 * a(0, 2) = 0.2;                                   // Stored as (2,0).
 * std::cout << std::as_const(a)(2, 0) << std::endl; // 0.2
 * @endcode
 * @short Sparse Symmetric Matrix.
 */
class RSparseSymmetricMatrix : public RSparseMatrix
{
public:

	/**
	 * Construct a sparse symmetric matrix.
	 * @param size				Number of lines and columns.
	 */
	explicit RSparseSymmetricMatrix(std::size_t size);

	// Keep the overloads of the generic matrix
	using RSparseMatrix::clear;
	using RSparseMatrix::verifySize;

	/**
	 * Get the type of the matrix.
	 */
	[[nodiscard]] RMatrixType type() const override { return RMatrixType::SparseSymmetric; }

	/**
	 * Change the size of the matrix. The elements outside the new size are removed.
	 * @param newLines			New number of lines.
	 * @param newCols			New number of columns (must be equal to
	 *							newLines).
	 * @param fill				Not used (the new elements are null).
	 * @param val				Not used.
	 * @exception std::range_error The matrix is not square.
	 */
	void verifySize(std::size_t newLines, std::size_t newCols, bool fill, double val) override;

	/**
	 * Get an element (0 if it is not stored).
	 * @param i					Line.
	 * @param j					Column.
	 * @exception std::range_error The indexes are outside the matrix.
	 */
	double operator()(std::size_t i, std::size_t j) const override;

	/**
	 * Get an element. If it is not stored, a null element is created.
	 * @param i					Line.
	 * @param j					Column.
	 * @exception std::range_error The indexes are outside the matrix.
	 */
	double& operator()(std::size_t i, std::size_t j) override;
};


}  //-------- End of namespace evocomb ---------------------------------------------
