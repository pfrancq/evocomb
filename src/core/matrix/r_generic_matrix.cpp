/*

	EvoComb

	RGenericMatrix.cpp

	Generic Matrix class - Implementation.

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
#include <cstddef>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "matrix/r_generic_matrix.hpp"
#include "matrix/r_matrix.hpp"
using namespace evocomb;



//------------------------------------------------------------------------------
//
//    Class RGenericMatrix
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
RGenericMatrix::RGenericMatrix(const size_t size)
	: nbLines_(size), nbCols_(size)
{
}


//------------------------------------------------------------------------------
RGenericMatrix::RGenericMatrix(const size_t lines, const size_t cols)
	: nbLines_(lines), nbCols_(cols)
{
}


//------------------------------------------------------------------------------
RGenericMatrix::RGenericMatrix(const RGenericMatrix& matrix) = default;


//------------------------------------------------------------------------------
RGenericMatrix::~RGenericMatrix() = default;


//------------------------------------------------------------------------------
void RGenericMatrix::clear(const double val, bool)
{
	init(val);
}


//------------------------------------------------------------------------------
void RGenericMatrix::init(const double val)
{
	for(std::size_t i = 0; i < nbLines_; i++)
		for(std::size_t j = 0; j < nbCols_; j++)
			(*this)(i,j) = val;
}


//------------------------------------------------------------------------------
void RGenericMatrix::add(const size_t i, const size_t j, const double val)
{
	(*this)(i,j) += val;
}


//------------------------------------------------------------------------------
RGenericMatrix& RGenericMatrix::operator=(const RGenericMatrix& matrix) = default;



//------------------------------------------------------------------------------
//
// General Functions and operators
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
RMatrix evocomb::operator+(const RGenericMatrix& arg1, const RGenericMatrix& arg2)
{
	if (arg1.nbLines()!=arg2.nbLines() || arg1.nbCols()!=arg2.nbCols())
		throw std::range_error("operator+(const RGenericMatrix&,const RGenericMatrix&) : Not Compatible Sizes");

	RMatrix res(arg1.nbLines(), arg1.nbCols());
	for (size_t i = 0; i < arg1.nbLines(); i++)
		for (size_t j = 0; j < arg1.nbCols(); j++)
			res(i,j) = arg1(i,j) + arg2(i,j);
	return res;
}


//------------------------------------------------------------------------------
RMatrix evocomb::operator-(const RGenericMatrix& arg1, const RGenericMatrix& arg2)
{
	if (arg1.nbLines()!=arg2.nbLines() || arg1.nbCols()!=arg2.nbCols())
		throw std::range_error("operator-(const RGenericMatrix&,const RGenericMatrix&) : Not Compatible Sizes");

	RMatrix res(arg1.nbLines(), arg1.nbCols());
	for (size_t i = 0; i < arg1.nbLines(); i++)
		for (size_t j = 0; j < arg1.nbCols(); j++)
			res(i,j) = arg1(i,j) - arg2(i,j);
	return res;
}

//------------------------------------------------------------------------------
RMatrix evocomb::operator*(const double arg1, const RGenericMatrix& arg2)
{
	RMatrix res(arg2.nbLines(), arg2.nbCols());
	for (size_t i = 0; i < arg2.nbLines(); i++)
		for (size_t j = 0; j < arg2.nbCols(); j++)
			res(i,j) = arg1 * arg2(i,j);
	return res;
}


//------------------------------------------------------------------------------
RMatrix evocomb::operator*(const RGenericMatrix& arg1,const double arg2)
{
	RMatrix res(arg1.nbLines(), arg1.nbCols());
	for (size_t i = 0; i < arg1.nbLines(); i++)
		for (size_t j = 0; j < arg1.nbCols(); j++)
			res(i,j) = arg1(i,j) * arg2;
	return res;
}


//------------------------------------------------------------------------------
RMatrix evocomb::operator*(const RGenericMatrix& arg1,const RGenericMatrix& arg2)
{
	if (arg1.nbCols() != arg2.nbLines())
		throw std::range_error("operator*(const RGenericMatrix&,const RGenericMatrix&) : Not Compatible Sizes");


	RMatrix res(arg1.nbLines(),arg2.nbCols());
	for (size_t i = 0; i < arg1.nbLines(); i++)
	{
		for (size_t j = 0; j < arg2.nbCols(); j++)
		{
			double Sum(0);
			for (size_t r = 0; r < arg1.nbCols(); r++)
				Sum += arg1(i,r) * arg2(r,j);
			res(i,j) = Sum;
		}
	}
	return res;
}
