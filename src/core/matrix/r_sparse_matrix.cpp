/*

	EvoComb

	RSparseMatrix.cpp

	Sparse Matrix - Implementation.

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
// include files for ANSI C/C++
#include <numeric>
#include <stdexcept>
#include <string>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "matrix/r_sparse_matrix.hpp"
using namespace evocomb;



//------------------------------------------------------------------------------
//
// class RSparseMatrix
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
RSparseMatrix::RSparseMatrix(const std::size_t nbLines, const std::size_t nbCols)
	: RGenericMatrix(nbLines, nbCols)
{
	lines_.reserve(nbLines);
	for (std::size_t i = 0; i < nbLines; i++)
		lines_.emplace_back(i);
}


//------------------------------------------------------------------------------
void RSparseMatrix::checkRange(const std::size_t i, const std::size_t j, const char* where) const
{
	if (i >= nbLines_ || j >= nbCols_)
		throw std::range_error(std::string(where) + " : index " + std::to_string(i) + "," + std::to_string(j) + " outside range (" + std::to_string(nbLines_) + "," + std::to_string(nbCols_) + ")");
}


//------------------------------------------------------------------------------
void RSparseMatrix::clear(double, const bool clean)
{
	for (auto& line : lines_)
	{
		line.clear();
		if (clean)
			line.values_.shrink_to_fit();
	}
}


//------------------------------------------------------------------------------
void RSparseMatrix::init(const double val)
{
	clear(val, false);
	if (val == 0.0)
		return;
	RGenericMatrix::init(val);
}


//------------------------------------------------------------------------------
void RSparseMatrix::verifySize(const std::size_t newLines, const std::size_t newCols, bool, double)
{
	if (newLines < lines_.size())
		lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(newLines), lines_.end());
	else
	{
		lines_.reserve(newLines);
		for (auto i = lines_.size(); i < newLines; i++)
			lines_.emplace_back(i);
	}
	if (newCols < nbCols_)
		for (auto& line : lines_)
			line.truncate(newCols);
	nbLines_ = newLines;
	nbCols_ = newCols;
}


//------------------------------------------------------------------------------
double RSparseMatrix::operator()(const std::size_t i, const std::size_t j) const
{
	checkRange(i, j, "RSparseMatrix::operator() const");
	return lines_[i][j];
}


//------------------------------------------------------------------------------
double& RSparseMatrix::operator()(const std::size_t i, const std::size_t j)
{
	checkRange(i, j, "RSparseMatrix::operator()");
	return lines_[i][j];
}


//------------------------------------------------------------------------------
const RSparseVector& RSparseMatrix::operator[](const std::size_t i) const
{
	if (i >= nbLines_)
		throw std::range_error("RSparseMatrix::operator[] const : index " + std::to_string(i) + " outside range (0," + std::to_string(nbLines_) + ")");
	return lines_[i];
}


//------------------------------------------------------------------------------
RSparseVector& RSparseMatrix::operator[](const std::size_t i)
{
	if (i >= nbLines_)
		throw std::range_error("RSparseMatrix::operator[] : index " + std::to_string(i) + " outside range (0," + std::to_string(nbLines_) + ")");
	return lines_[i];
}


//------------------------------------------------------------------------------
std::size_t RSparseMatrix::nbValues() const
{
	return std::accumulate(lines_.begin(), lines_.end(), std::size_t{0}, [](const std::size_t sum, const RSparseVector& line) {
		return sum + line.size();
	});
}


//------------------------------------------------------------------------------
void RSparseMatrix::removeNulls()
{
	for (auto& line : lines_)
		line.removeNulls();
}


//------------------------------------------------------------------------------
bool RSparseMatrix::operator==(const RSparseMatrix& matrix) const
{
	return type() == matrix.type() && nbLines_ == matrix.nbLines_ && nbCols_ == matrix.nbCols_ && lines_ == matrix.lines_;
}
