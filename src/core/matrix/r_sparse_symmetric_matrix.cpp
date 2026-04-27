/*

	EvoComb

	RSparseSymmetricMatrix.cpp

	Sparse Symmetric Matrix - Implementation.

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
// include files for ANSI C/C++
#include <stdexcept>
#include <utility>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "matrix/r_sparse_symmetric_matrix.hpp"
using namespace evocomb;



//------------------------------------------------------------------------------
//
// class RSparseSymmetricMatrix
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
RSparseSymmetricMatrix::RSparseSymmetricMatrix(const std::size_t size)
	: RSparseMatrix(size, size)
{
}


//------------------------------------------------------------------------------
void RSparseSymmetricMatrix::verifySize(const std::size_t newLines, const std::size_t newCols, const bool fill, const double val)
{
	if (newLines != newCols)
		throw std::range_error("RSparseSymmetricMatrix::verifySize : matrix must be square");
	RSparseMatrix::verifySize(newLines, newCols, fill, val);
}


//------------------------------------------------------------------------------
double RSparseSymmetricMatrix::operator()(std::size_t i, std::size_t j) const
{
	checkRange(i, j, "RSparseSymmetricMatrix::operator() const");
	if (j > i)
		std::swap(i, j);
	return lines_[i][j];
}


//------------------------------------------------------------------------------
double& RSparseSymmetricMatrix::operator()(std::size_t i, std::size_t j)
{
	checkRange(i, j, "RSparseSymmetricMatrix::operator()");
	if (j > i)
		std::swap(i, j);
	return lines_[i][j];
}
