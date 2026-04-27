	/*

	EvoComb

	RLowerTriangularMatrix.cpp

	Matrix class - Implementation.

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
// include files for ANSI C/C++
#include <cstddef>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "matrix/r_lower_triangular_matrix.hpp"
using namespace evocomb;



//------------------------------------------------------------------------------
//
//    Class RLowerTriangularMatrix
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
RLowerTriangularMatrix::RLowerTriangularMatrix(const size_t size)
	: RMatrix(size, size, false)
{
	lines_.reserve(size);
	for (std::size_t i = 0; i < size; i++)
		lines_.emplace_back(i + 1);
}


//------------------------------------------------------------------------------
RLowerTriangularMatrix::RLowerTriangularMatrix(const std::vector<std::vector<double>>& matrix):
	RMatrix(matrix.size(), matrix.size(), false)
{
	lines_.reserve(matrix.size());
	size_t nbCols = 1;
	for (auto& line: matrix)
	{
		if (line.size() != nbCols)
			throw std::range_error("RMatrix::RMatrix() : Not a lower triangular matrix");
		lines_.emplace_back(line.size());
		auto col = lines_.back().begin();
		for (auto row = line.begin(); row != line.end(); ++row, ++col)
			*col = *row;
		nbCols++;
	}
}


//------------------------------------------------------------------------------
void RLowerTriangularMatrix::verifySize(const size_t newlines, const size_t newCols, const bool fill, const double val)
{
	if (newlines != newCols)
		throw std::range_error("RLowerTriangularMatrix::VerifySize(size_t,size_t) : Matrix must be squared");

	// Verify the lines
	if (newlines > nbLines_)
	{
		// New lines must be added.
		lines_.reserve(newlines);
		for (auto i = nbLines_; i < newlines; i++)
		{
			lines_.emplace_back(i + 1);
			if (fill)
			{
				auto& col = lines_.back();
				for (std::size_t j = 0; j <= i; j++)
					col[j] = val;
			}
		}
	}
	else if (newlines < nbLines_)
	{
		// A number of lines must be deleted
		for (auto i= nbLines_ - newlines + 1; --i;)
			lines_.pop_back();
	}

	nbLines_ = newlines;
	nbCols_ = newCols;
}


//------------------------------------------------------------------------------
double RLowerTriangularMatrix::operator()(const size_t i, const size_t j) const
{
	if ( i >= nbLines_ || j >= nbCols_)
		throw std::range_error("RLowerTriangularMatrix::operator() const : index " + std::to_string(i) + "," + std::to_string(j) + " outside range (" + std::to_string(nbLines_) + "," + std::to_string(nbCols_) + ")");
	if (j > i)
		return 0.0;
	return lines_[i][j];
}


//------------------------------------------------------------------------------
double& RLowerTriangularMatrix::operator()(const size_t i, const size_t j)
{
	if (i >= nbLines_ || j >= nbCols_)
		throw std::range_error("RLowerTriangularMatrix::operator() : index " + std::to_string(i) + "," + std::to_string(j) + " outside range (" + std::to_string(nbLines_) + "," + std::to_string(nbCols_) + ")");
	if (j > i)
		throw std::range_error("RLowerTriangularMatrix::operator() : Invalid column " + std::to_string(j) + " for line " + std::to_string(i));
	return lines_[i][j];
}
