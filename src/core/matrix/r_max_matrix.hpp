/*

	EvoComb

	RMaxMatrix.hpp

	Sparse Matrix of Vectors Ordered by Decreasing Values - Header.

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
#include "matrix/r_max_vector.hpp"


//------------------------------------------------------------------------------
namespace evocomb{
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
 * The RMaxMatrix class provides a sparse matrix where each line is a vector ordered by decreasing values (see
 * RMaxVector) that keeps at most nbCols() values. It is typically used to store, for each object, its nbCols() nearest
 * neighbors.
 *
 * The values are added with add(i, id, value), where id is the identifier associated to the value (for example a
 * neighbor). As a generic matrix, the element (i,j) is the j-th greatest value of line i (0 if the line has less than
 * j+1 values). So, the generic operators (see RGenericMatrix) work on the values ordered by rank. The elements cannot
 * be modified through operator(): use add. Be careful that, on a non-const matrix, m(i,j) calls the non-const version
 * of operator(), which throws an exception: use a const reference (or std::as_const(m)(i,j)).
 * @code
 * evocomb::RMaxMatrix sims(nbObjs, 10);        // The 10 nearest neighbors.
 * sims.add(0, 5, 0.9);                     // Object 5 is similar to object 0.
 * for (const auto& [id, sim] : sims[0])
 *     std::cout << id << ": " << sim << std::endl;
 * @endcode
 * @short Sparse Matrix of Ordered Vectors.
 */
class RMaxMatrix : public RGenericMatrix
{
public:

	/**
	 * Construct the matrix.
	 * @param nbLines			Number of lines.
	 * @param nbCols			Maximal number of values of each line (0 means
	 *							no limit).
	 */
	RMaxMatrix(std::size_t nbLines, std::size_t nbCols);

	// Keep the overloads of the generic matrix
	using RGenericMatrix::clear;
	using RGenericMatrix::verifySize;

	/**
	 * Get the type of the matrix.
	 */
	[[nodiscard]] RMatrixType type() const override { return RMatrixType::Max; }

	/**
	 * Remove all the values (the size of the matrix is not modified).
	 * @param val				Not used.
	 * @param clean				Not used.
	 */
	void clear(double val, bool clean) override;

	/**
	 * Not valid for this matrix (use add).
	 * @param val				Value.
	 * @exception RException	Always.
	 */
	void init(double val) override;

	/**
	 * Change the size of the matrix. If the number of columns decreases, the smallest values of each line are removed.
	 * @param newLines			New number of lines.
	 * @param newCols			New maximal number of values of each line.
	 * @param fill				Not used.
	 * @param val				Not used.
	 */
	void verifySize(std::size_t newLines, std::size_t newCols, bool fill, double val) override;

	/**
	 * Get the j-th greatest value of line i (0 if the line has less than j+1 values).
	 * @param i					Line.
	 * @param j					Rank of the value.
	 * @exception std::range_error The indexes are outside the matrix.
	 */
	double operator()(std::size_t i, std::size_t j) const override;

	/**
	 * Not valid for this matrix (use add).
	 * @param i					Line.
	 * @param j					Rank of the value.
	 * @exception RException	Always.
	 */
	double& operator()(std::size_t i, std::size_t j) override;

	/**
	 * Set the value associated to an identifier in a line (if the line is full, the value is only inserted if it is
	 * greater than the smallest one, which is then removed). Contrary to RGenericMatrix::add, the value is not added to
	 * an existing one but replaces it.
	 * @param i					Line.
	 * @param id				Identifier associated to the value.
	 * @param val				Value.
	 * @exception std::range_error The line is outside the matrix.
	 */
	void add(std::size_t i, std::size_t id, double val) override;

	/**
	 * Get a line.
	 * @param i					Line.
	 * @exception std::range_error The line is outside the matrix.
	 */
	[[nodiscard]] const RMaxVector& operator[](std::size_t i) const;

	/**
	 * Get a line.
	 * @param i					Line.
	 * @exception std::range_error The line is outside the matrix.
	 */
	[[nodiscard]] RMaxVector& operator[](std::size_t i);

	/**
	 * Get all the lines.
	 */
	[[nodiscard]] const std::vector<RMaxVector>& lines() const { return lines_; }

private:

	//-----------------------------------------------------
	std::vector<RMaxVector> lines_;			///< Lines (the identifier of line i is i).
};


}  //-------- End of namespace evocomb ---------------------------------------------
