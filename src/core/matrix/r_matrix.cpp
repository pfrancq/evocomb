/*

	EvoComb

	RMatrix.cpp

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
#include "matrix/r_matrix.hpp"
using namespace evocomb;



//------------------------------------------------------------------------------
//
//    Class RMatrix
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
RMatrix::RMatrix(const size_t size):
	RGenericMatrix(size)
{
	lines_.reserve(size);
	for(std::size_t i = 0; i < size; i++)
		lines_.emplace_back(size);
}


//------------------------------------------------------------------------------
RMatrix::RMatrix(const size_t lines, const size_t cols):
	RGenericMatrix(lines, cols)
{
	lines_.reserve(lines);
	for(std::size_t i = 0; i < lines; i++)
		lines_.emplace_back(cols);
}


//------------------------------------------------------------------------------
RMatrix::RMatrix(const size_t lines, const size_t cols, bool):
	RGenericMatrix(lines, cols)
{
}


//------------------------------------------------------------------------------
RMatrix::RMatrix(const std::vector<std::vector<double>>& matrix):
	RGenericMatrix(matrix.size(), matrix.empty() ? 0 : matrix[0].size())
{
	lines_.reserve(matrix.size());
	for (auto& line: matrix)
	{
		if (line.size() != nbCols_)
			throw std::range_error("RMatrix::RMatrix() : Not a matrix");
		lines_.emplace_back(line.size());
		auto col = lines_.back().begin();
		for (auto row = line.begin(); row != line.end(); ++row, ++col)
			*col = *row;
	}
}


//------------------------------------------------------------------------------
void RMatrix::init(const double val)
{
	for(auto& line: lines_)
		for (auto& col: line)
			col = val;
}


//------------------------------------------------------------------------------
void RMatrix::verifySize(const size_t newLines, const size_t newCols, const bool fill, const double val)
{
	// Verify the lines
	if (newLines > nbLines_)
	{
		// New lines must be added.
		lines_.reserve(newLines);
		for (auto i = nbLines_; i < newLines; i++)
		{
			lines_.emplace_back(newCols);
			if(fill)
			{
				for(auto& col: lines_.back())
					col = val;
			}
		}
	}
	else if (newLines < nbLines_)
		lines_.erase(lines_.begin() + static_cast<long>(newLines), lines_.end());

	// Verify the columns
	if (newCols > nbCols_)
	{
		// Each vector must be expanded
		for (auto& line: lines_)
		{
			line.resize(newCols);
			if(fill)
			{
				for(auto& col: lines_.back())
					col = val;
			}
		}
	}
	else if(newCols < nbCols_)
	{
		// Each vector must be resized
		for (auto& line: lines_)
			line.resize(newCols);
	}

	nbLines_ = newLines;
	nbCols_ = newCols;
}


//------------------------------------------------------------------------------
void RMatrix::symmetrize()
{
	if (nbCols_ != lines_.size())
		throw std::range_error("RMatrix::Symmetrize() : Not a symmetric matrix");

	for (std::size_t i = 0 ; i < nbLines_; i++)
		for (std::size_t j = 0 ; j < i; j++)
			lines_[j][i] = lines_[i][j];
}


//------------------------------------------------------------------------------
double RMatrix::operator()(const size_t i, const size_t j) const
{
	if (i >= nbLines_ || j >= nbCols_)
		throw std::range_error("RMatrix::operator(size_t,size_t) const : index " + std::to_string(i) + "," + std::to_string(j) + " outside range (" + std::to_string(nbLines_) + "," + std::to_string(nbCols_) + ")");
	return lines_[i][j];
}


//------------------------------------------------------------------------------
double& RMatrix::operator()(const size_t i, const size_t j)
{
	if (i >= nbLines_ || j >= nbCols_)
		throw std::range_error("RMatrix::operator(size_t,size_t) : index " + std::to_string(i) + "," + std::to_string(j) + " outside range (" + std::to_string(nbLines_) + "," + std::to_string(nbCols_) + ")");
	return lines_[i][j];
}


//------------------------------------------------------------------------------
RMatrix& RMatrix::operator+=(const RMatrix& matrix)
{
	if (nbLines_ != matrix.nbLines_ || nbCols_ != matrix.nbCols_)
		throw std::range_error("RMatrix::operator+= : Not Compatible Sizes");
	for (std::size_t i = 0 ; i < nbLines_; i++)
		for (std::size_t j = 0 ; j < nbCols_; j++)
			lines_[i][j] += matrix(i,j);
	return *this;
}


//------------------------------------------------------------------------------
RMatrix& RMatrix::operator-=(const RMatrix& matrix)
{
	if (nbLines_ != matrix.nbLines_ || nbCols_ != matrix.nbCols_)
		throw std::range_error("RMatrix::operator-= : Not Compatible Sizes");
	for (std::size_t i = 0 ; i < nbLines_; i++)
		for (std::size_t j = 0 ; j < nbCols_; j++)
			lines_[i][j] -= matrix(i,j);
	return *this;
}


//------------------------------------------------------------------------------
RMatrix& RMatrix::operator*=(const double arg)
{
	for (auto& line: lines_)
		for (auto& col: line)
			col *= arg;
	return *this;
}


//------------------------------------------------------------------------------
RMatrix& RMatrix::operator/=(const double arg)
{
	for (auto& line: lines_)
		for (auto& col: line)
			col /= arg;
	return *this;
}


//------------------------------------------------------------------------------
RMatrix& RMatrix::operator*=(const RMatrix& matrix)
{
	if (nbCols_ != matrix.lines_.size())
		throw std::range_error("RMatrix::operator*= : Not Compatible Sizes");

	// Copy in res the results of the product
	RMatrix res(lines_.size(), matrix.nbCols_);
	for (std::size_t i = 0; i < lines_.size(); ++i)
	{
		for (std::size_t j = 0; j < matrix.nbCols_; ++j)
		{
			res(i,j) = 0;
			for (std::size_t k = 0; k < nbCols_; ++k)
				res(i,j) += (*this)(i, k) * matrix(k, j);
		}
	}

	// Copy res in this
	*this = res;
	return *this;
}



//------------------------------------------------------------------------------
//
// General Functions and operators
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
RMatrix evocomb::operator+(const RMatrix& arg1,const RMatrix& arg2)
{
	RMatrix res(arg1);
	res += arg2;
	return res;
}


//------------------------------------------------------------------------------
RMatrix evocomb::operator-(const RMatrix& arg1,const RMatrix& arg2)
{
	RMatrix res(arg1);
	res -= arg2;
	return res;
}


//------------------------------------------------------------------------------
RMatrix evocomb::operator*(const double arg1,const RMatrix& arg2)
{
	RMatrix res(arg2);
	res *= arg1;
	return res;
}


//------------------------------------------------------------------------------
RMatrix evocomb::operator/(const double arg1,const RMatrix& arg2)
{
	RMatrix res(arg2);
	res /= arg1;
	return res;
}


//------------------------------------------------------------------------------
RMatrix evocomb::operator/(const RMatrix& arg1,const double arg2)
{
	RMatrix res(arg1);
	res /= arg2;
	return res;
}


//------------------------------------------------------------------------------
RMatrix evocomb::operator*(const RMatrix& arg1,const RMatrix& arg2)
{
	RMatrix res(arg1);
	res *= arg2;
	return res;
}
