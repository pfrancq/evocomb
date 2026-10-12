/*

	EvoComb

	RSparseMatrix.hpp

	Sparse Matrix - Header.

	Copyright 2005-2026 by Pascal Francq (pascal@francq.info).
	Copyright 2003-2005 by Valery Vandaele.
	Copyright 2003-2008 by the Université Libre de Bruxelles (ULB).

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
#include <vector>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "matrix/r_generic_matrix.hpp"
#include "matrix/r_sparse_vector.hpp"


//------------------------------------------------------------------------------
namespace evocomb{
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
 * The RSparseMatrix class provides a sparse matrix: each line is a sparse vector (see RSparseVector), and the elements
 * that are not stored are null.
 * @code
 * evocomb::RSparseMatrix a(6, 6);
 * a(0, 2) = 0.2;                       // Creates the element (0,2).
 * a.set(1, 5, 1.5);
 * a.add(2, 0, 1.0);
 * for (std::size_t i = 0; i < a.nbLines(); i++)
 * {
 *     for (std::size_t j = 0; j < a.nbCols(); j++)
 *         std::cout << std::as_const(a)(i, j) << "\t";
 *     std::cout << std::endl;
 * }
 * for (const auto& [j, value] : a[1])  // Only the elements stored.
 *     std::cout << j << ": " << value << std::endl;
 * @endcode
 * As for std::map, the non-const operator() creates a null element if it does not exist: to only read, use a const
 * reference (or std::as_const(a)(i,j)).
 * @short Sparse Matrix.
 */
class RSparseMatrix : public RGenericMatrix
{
public:

	/**
	 * Construct a sparse matrix.
	 * @param nbLines			Number of lines.
	 * @param nbCols			Number of columns.
	 */
	RSparseMatrix(std::size_t nbLines, std::size_t nbCols);

	// Keep the overloads of the generic matrix
	using RGenericMatrix::clear;
	using RGenericMatrix::verifySize;

	/**
	 * Get the type of the matrix.
	 */
	[[nodiscard]] RMatrixType type() const override { return RMatrixType::Sparse; }

	/**
	 * Remove all the elements (the size of the matrix is not modified).
	 * @param val				Not used.
	 * @param clean				Release also the memory.
	 */
	void clear(double val, bool clean) override;

	/**
	 * Set all the elements of the matrix to a given value. If the value is not null, all the elements are created (use
	 * RMatrix instead).
	 * @param val				Value.
	 */
	void init(double val) override;

	/**
	 * Change the size of the matrix. The elements outside the new size are removed.
	 * @param newLines			New number of lines.
	 * @param newCols			New number of columns.
	 * @param fill				Not used (the new elements are null).
	 * @param val				Not used.
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

	/**
	 * Set an element.
	 * @param i					Line.
	 * @param j					Column.
	 * @param val				Value.
	 * @exception std::range_error The indexes are outside the matrix.
	 */
	void set(const std::size_t i, const std::size_t j, const double val) { (*this)(i, j) = val; }

	/**
	 * Add a value to an element.
	 * @param i					Line.
	 * @param j					Column.
	 * @param val				Value.
	 * @exception std::range_error The indexes are outside the matrix.
	 */
	void add(std::size_t i, std::size_t j, double val) override { (*this)(i, j) += val; }

	/**
	 * Get a line.
	 * @param i					Line.
	 * @exception std::range_error The line is outside the matrix.
	 */
	[[nodiscard]] const RSparseVector& operator[](std::size_t i) const;

	/**
	 * Get a line. Be careful to not create elements outside the matrix.
	 * @param i					Line.
	 * @exception std::range_error The line is outside the matrix.
	 */
	[[nodiscard]] RSparseVector& operator[](std::size_t i);

	/**
	 * Get all the lines (the identifier of line i is i).
	 */
	[[nodiscard]] const std::vector<RSparseVector>& lines() const { return lines_; }

	/**
	 * Get the number of elements stored.
	 */
	[[nodiscard]] std::size_t nbValues() const;

	/**
	 * Remove all the null elements stored.
	 */
	void removeNulls();

	/**
	 * Compare two sparse matrices (sizes and elements stored).
	 * @param matrix			Matrix.
	 */
	[[nodiscard]] bool operator==(const RSparseMatrix& matrix) const;

	/**
	 * Compare two sparse matrices.
	 * @param matrix			Matrix.
	 */
	[[nodiscard]] bool operator!=(const RSparseMatrix& matrix) const { return !(*this == matrix); }

protected:

	/**
	 * Verify that an element is inside the matrix.
	 * @param i					Line.
	 * @param j					Column.
	 * @param where				Method calling.
	 * @exception std::range_error The indexes are outside the matrix.
	 */
	void checkRange(std::size_t i, std::size_t j, const char* where) const;

	//-----------------------------------------------------
	std::vector<RSparseVector> lines_;		///< Lines.
};


}  //-------- End of namespace evocomb ---------------------------------------------
