/*

	EvoComb

	RGenericMatrix.hpp

	Generic Matrix class - Header.

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
// include files for ANSI C/C++
#include <cmath>
#include <string>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "tools/r_std.hpp"
#include "matrix/r_vector.hpp"


//------------------------------------------------------------------------------
namespace evocomb{
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Forward declaration
class RMatrix;


//------------------------------------------------------------------------------
/**
 * Type of matrix.
 */
enum class RMatrixType
{
	Normal					/** Normal matrix.*/,
	Diagonal				/** Diagonal matrix.*/,
	LowerTriangular			/** Lower triangular matrix.*/,
	UpperTriangular			/** Upper triangular matrix.*/,
	Symmetric				/** Symmetric matrix.*/,
	Sparse					/** Sparse matrix.*/,
	SparseSymmetric			/** Sparse symmetric matrix.*/,
	Max						/** Ascending ordered sparse matrix.*/
};


//------------------------------------------------------------------------------
/**
* The RGenericMatrix class provides a representation for a generic matrix. It provides several operator acting on
* generic matrices. The operators are based on the operator(i,j) and are not optimized. If possible, these operators
* should be specialized in inheriting classes for performance purposes.
* @short Generic Matrix.
*/
class RGenericMatrix
{
public:

	/**
	* Construct a square matrix of a given size.
	* @param size            Size of the matrix.
	*/
	explicit RGenericMatrix(size_t size);

	/**
	* Construct a matrix (L,C).
	* @param lines           Number of lines.
	* @param cols            Number of columns.
	*/
	RGenericMatrix(size_t lines, size_t cols);

	/**
	* Construct a matrix from another one.
	* @param matrix          Matrix.
	*/
	RGenericMatrix(const RGenericMatrix& matrix);

	/**
	* Destruct the matrix.
	*/
	virtual ~RGenericMatrix();

	/**
	 * Clear the matrix.
	 * @param val            Value eventually used to assign to the elements.
	 * @param clean          Clean eventually the memory.
	 */
	virtual void clear(double val, bool clean);

	/**
	 * Clear the matrix.
	 * @param val            Value eventually used to assign to the elements.
	 */
	virtual void clear(const double val) { clear(val,false); }

	/**
	 * Clear the matrix.
	 */
	void clear() { clear(NAN, false); }

	/**
	 * Initialize the matrix with a given value. It uses the operator(i,j) to initialize the different elements.
	 * @param val            Value to assign.
	 */
	virtual void init(double val);

	/**
	* Verify if the matrix has a given size, and increase them if necessary.
	* @param newlines        New line number.
	* @param newCols         New column number.
	* @param fill            Elements must be filled with a value.
	* @param val             Value used eventually to fill the elements created.
	*/
	virtual void verifySize(size_t newlines, size_t newCols, bool fill, double val) = 0;

	/**
	* Verify if the matrix has a given size, and increase them if necessary.
	* @param newlines        New line number.
	* @param newCols         New column number.
	* @param fill            Elements must be filled with a value.
	*/
	void verifySize(const size_t newlines, const size_t newCols, const bool fill) { verifySize(newlines, newCols, fill, NAN); }

	/**
	* Verify if the matrix has a given size, and increase them if necessary.
	* @param newlines        New line number.
	* @param newCols         New column number.
	*/
	void verifySize(const size_t newlines, const size_t newCols) { verifySize(newlines, newCols, false); }

	/**
	 * Get the type of the matrix.
	 */
	[[nodiscard]] virtual RMatrixType type() const=0;

	/**
	* Get the number of lines in the matrix.
	*/
	[[nodiscard]] size_t nbLines() const {return nbLines_; }

	/**
	* Get the number of columns in the matrix.
	*/
	[[nodiscard]] size_t nbCols() const { return nbCols_; }

	/**
	* Return a specific element of the matrix (const version).
	* @param i               Line number of the element.
	* @param j               Column number of the element.
	*/
 	virtual double operator()(size_t i, size_t j) const = 0;

	/**
	* Return a specific element of the matrix (const version).
	* @param i               Line number of the element.
	* @param j               Column number of the element.
	*/
 	virtual double& operator()(size_t i, size_t j)=0;

	/**
	 * Add a value associated to a given index into the matrix. By default, it calls the operator(size_t,size_t) method
	 * @param i              Line number of the element.
	 * @param j              Column number of the element.
	 * @param val            Value to add.
	 */
	virtual void add(size_t i, size_t j,double val);

	/**
	* Assign operator.
	* @param matrix          Matrix.
	*/
	RGenericMatrix& operator=(const RGenericMatrix& matrix);

	/**
	 * Print the content of the matrix.
	 * @tparam S				Stream class that implements the << operator.
	 * @param stream			Stream.
	 * @param name				Name of the vector.
	 * @param precision			Precision.
	 * @param scientific		Scientific notation.
	 * @param colSize			Size of a column. By default, it is 12.
	 */
	template<class S> void print(S& stream, const std::string& name, const int precision = 2, const bool scientific = true, const int colSize=12) const
	{
		if (scientific)
			stream << std::scientific;
		stream << std::setprecision(precision);
		std::string Spaces;
		for (size_t i=0 ; i<name.size() + 1; i++)
			Spaces += ' ';
		const auto namePos(nbLines_ / 2);

		for(auto i = 0; i < nbLines_; i++)
		{
			if (namePos == i)
			{
				stream << name << "=";
				if (i == nbLines_ - 1)
					stream << "└ ";
				else
					stream << "│ ";
			}
			else
			{
				stream << Spaces;
				if (!i)
					stream << "┌ ";
				else if (i == nbLines_ - 1)
					stream << "└ ";
				else
					stream << "│ ";
			}
			for (auto j=0 ; j < nbCols_; j++)
				stream << std::setw(colSize) << std::left << (*this)(i,j);
			if (!i)
				stream << "┐";
			else if(i == nbLines_ - 1)
				stream << "┘";
			else
				stream << "│";
			stream << std::endl;
		}
	}

protected:

	//-----------------------------------------------------
	size_t nbLines_;				///< Number of lines.
	size_t nbCols_;					///< Number of columns.

};


//------------------------------------------------------------------------------
/**
* Addition of two matrixes.
* @param arg1                Matrix.
* @param arg2                Matrix.
*/
RMatrix operator+(const RGenericMatrix& arg1, const RGenericMatrix& arg2);


//------------------------------------------------------------------------------
/**
* Subtraction of two matrixes.
* @param arg1                Matrix.
* @param arg2                Matrix.
*/
RMatrix operator-(const RGenericMatrix& arg1, const RGenericMatrix& arg2);


//------------------------------------------------------------------------------
/**
* Multiplication of a matrix and a number.
* @param arg1                Matrix.
* @param arg2                Number.
*/
RMatrix operator*(const RGenericMatrix& arg1, double arg2);


//------------------------------------------------------------------------------
/**
* Multiplication of a matrix and a number.
* @param arg1                Number.
* @param arg2                Matrix.
*/
RMatrix operator*(double arg1, const RGenericMatrix& arg2);


//------------------------------------------------------------------------------
/**
* Multiplication of two matrixes.
* @param arg1                Matrix.
* @param arg2                Matrix.
*/
RMatrix operator*(const RGenericMatrix& arg1, const RGenericMatrix& arg2);


}  //-------- End of namespace evocomb -----------------------------------------------
