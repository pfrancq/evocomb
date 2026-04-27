/*

	EvoComb

	RVector.cpp

	Class representing a vector - Implementation

	Copyright 2008-2015 by Pascal Francq (pascal@francq.info).

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
#include <cmath>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "matrix/r_vector.hpp"
using namespace evocomb;



//------------------------------------------------------------------------------
//
// class RVector
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
RVector::RVector(const size_t size)
{
	values_.resize(size);
}


//------------------------------------------------------------------------------
RVector::RVector(std::vector<double>&& values):
	values_(std::move(values))
{
}


//------------------------------------------------------------------------------
void RVector::init(const double val)
{
	std::fill(values_.begin(), values_.end(), val);
}


//------------------------------------------------------------------------------
void RVector::resize(const size_t newSize, const double val)
{
	// The new values (if any) are initialized with val
	values_.resize(newSize, val);
}


//------------------------------------------------------------------------------
RVector& RVector::operator+=(const RVector& vector)
{
	if (values_.size() != vector.values_.size())
		throw std::range_error("RVector::operator+=(const RVector&) : Not Compatible Sizes");
	auto it = values_.begin();
	auto it2 = vector.values_.begin();
	for(; it!=values_.end() ; ++it, ++it2)
		*it += *it2;
	return *this;
}


//------------------------------------------------------------------------------
RVector& RVector::operator-=(const RVector& vector)
{
	if (values_.size() != vector.values_.size())
		throw std::range_error("RVector::operator-=(const RVector&) : Not Compatible Sizes");
	auto it = values_.begin();
	auto it2 = vector.values_.begin();
	for(; it!=values_.end() ; ++it, ++it2)
		*it -= *it2;
	return *this;
}


//------------------------------------------------------------------------------
RVector& RVector::operator*=(const double arg)
{
	for (auto& element : values_)
		element *= arg;
	return *this;
}


//------------------------------------------------------------------------------
RVector& RVector::operator/=(const double arg)
{
	for (auto& element : values_)
		element /= arg;
	return *this;
}


//------------------------------------------------------------------------------
double RVector::operator*(const RVector& vector) const
{
	if (values_.size() != vector.values_.size())
		throw std::range_error("R::operator*(const RVector&,const RVector&) : Not Compatible Sizes");

	double Sum(0.0);
	double Norm1(0.0);
	double Norm2(0.0);
	for (std::size_t i = 0; i < values_.size(); i++)
	{
		Sum += values_[i] * vector.values_[i];
		Norm1 += values_[i] * vector.values_[i];
		Norm2 += values_[i] * vector.values_[i];
	}
	if (Sum == 0.0)
		return 0.0;
	return Sum / (sqrt(Norm1) * sqrt(Norm2));
}



//------------------------------------------------------------------------------
//
// General Functions and operators
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
RVector evocomb::operator+(const RVector &arg1, const RVector &arg2)
{
	RVector res(arg1);
	res += arg2;
	return res;
}


//------------------------------------------------------------------------------
RVector evocomb::operator-(const RVector &arg1, const RVector &arg2)
{
	RVector res(arg1);
	res-=arg2;
	return res;
}


//------------------------------------------------------------------------------
RVector evocomb::operator*(const double arg1, const RVector& arg2)
{
	RVector res(arg2);
	res *= arg1;
	return res;
}


//------------------------------------------------------------------------------
RVector evocomb::operator*(const RVector& arg1, const double arg2)
{
	RVector res(arg1);
	res *= arg2;
	return res;
}


//------------------------------------------------------------------------------
RVector evocomb::operator/(const double arg1, const RVector& arg2)
{
	RVector res(arg2);
	res /= arg1;
	return res;
}


//------------------------------------------------------------------------------
RVector evocomb::operator/(const RVector& arg1, const double arg2)
{
	RVector res(arg1);
	res /= arg2;
	return res;
}


//------------------------------------------------------------------------------
double evocomb::operator*(const RVector& arg1, const RVector& arg2)
{
	return arg1.operator*(arg2);
}
