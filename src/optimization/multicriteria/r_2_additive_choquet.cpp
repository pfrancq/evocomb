/*

	EvoComb

	R2AdditiveChoquet.cpp

	The 2-additive Choquet Integral - Implementation.

	Copyright 2013-2026 by Pascal Francq (pascal@francq.info).

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
#include <stdexcept>
#include <utility>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "multicriteria/r_2_additive_choquet.hpp"
#include "matrix/r_generic_matrix.hpp"
using namespace evocomb;



//------------------------------------------------------------------------------
//
// class R2AdditiveChoquet
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
R2AdditiveChoquet::R2AdditiveChoquet(const size_t n, std::string name)
	: name_(std::move(name)), nbCriteria_(n), params_(n), verified_(false)
{
	if (!nbCriteria_)
		throw std::range_error("R2AdditiveChoquet::R2AdditiveChoquet : Null number of criteria");
}


//------------------------------------------------------------------------------
R2AdditiveChoquet::R2AdditiveChoquet(const size_t n, RSymmetricMatrix& params, std::string name)
	: name_(std::move(name)), nbCriteria_(n), params_(n), verified_(false)
{
	if (!nbCriteria_)
		throw std::range_error("R2AdditiveChoquet::R2AdditiveChoquet : Null number of criteria");
	setParams(params);
}


//------------------------------------------------------------------------------
bool R2AdditiveChoquet::verify()
{
	if (verified_)
		return true;
	double Sum(0.0);
	for (std::size_t i = 0; i < nbCriteria_; i++)
		for (auto j = i; j < nbCriteria_; j++)
			Sum += params_(i,j);
	verified_ = fabs(Sum - 1.0) < 0.00000000001;
	return verified_;
}


//------------------------------------------------------------------------------
void R2AdditiveChoquet::setParams(RSymmetricMatrix& params)
{
	// Verify the matrix
	if (params.nbCols()!=nbCriteria_)
		throw std::range_error("R2AdditiveChoquet::SetParams : The matrix is not of size (" + std::to_string(nbCriteria_) + "," + std::to_string(nbCriteria_) + ")");
	double Sum(0.0);
	for (std::size_t i = 0; i < nbCriteria_; i++)
		for (auto j = i; j < nbCriteria_; j++)
			Sum += params(i,j);
	verified_ = fabs(Sum - 1.0) < 0.00000000001;
	if (!verified_)
		throw std::invalid_argument("R2AdditiveChoquet::SetParams : The sum of the parameters is not 1");

	// Do the assignation
	params_ = params;
}


//------------------------------------------------------------------------------
void R2AdditiveChoquet::setCriteria(const size_t idx, const double weight)
{
	if (idx >= nbCriteria_)
		throw std::range_error("R2AdditiveChoquet::SetCriteria : " + std::to_string(idx) + " is not a valid criteria index");
	params_(idx,idx) = weight;
	verified_ = false;
}


//------------------------------------------------------------------------------
double R2AdditiveChoquet::criteria(const size_t idx) const
{
	if (idx >= nbCriteria_)
		throw std::range_error("R2AdditiveChoquet::GetCriteria : " + std::to_string(idx) + " is not a valid criteria index");
	return params_(idx,idx);
}


//------------------------------------------------------------------------------
void R2AdditiveChoquet::setInteraction(const size_t idx1, const size_t idx2, const double weight)
{
	if (idx1 >= nbCriteria_)
		throw std::range_error("R2AdditiveChoquet::SetInteraction : " + std::to_string(idx1) + " is not a valid criteria index");
	if (idx2 >= nbCriteria_)
		throw std::range_error("R2AdditiveChoquet::SetInteraction : " + std::to_string(idx2) + " is not a valid criteria index");
	if (idx1 == idx2)
		throw std::range_error("R2AdditiveChoquet::SetInteraction : Different criteria indexes are needed");
	params_(idx1,idx2)=weight;
	verified_ = false;
}


//------------------------------------------------------------------------------
double R2AdditiveChoquet::interaction(const size_t idx1, const size_t idx2) const
{
	if (idx1 >= nbCriteria_)
		throw std::range_error("R2AdditiveChoquet::GetInteraction : " + std::to_string(idx1) + " is not a valid criteria index");
	if (idx2 >= nbCriteria_)
		throw std::range_error("R2AdditiveChoquet::GetInteraction : " + std::to_string(idx2) + " is not a valid criteria index");
	if (idx1 == idx2)
		throw std::range_error("R2AdditiveChoquet::GetInteraction : Different criteria indexes are needed");
	return params_(idx1,idx2);
}


//------------------------------------------------------------------------------
double R2AdditiveChoquet::compute(const std::vector<double>& values)
{
	// Verifications
	if (!verify())
		throw std::invalid_argument("R2AdditiveChoquet::SetParams : The sum of the parameters is not 1");
	if (values.size() != nbCriteria_)
		throw std::range_error("R2AdditiveChoquet::Compute : The vector has not a size of " + std::to_string(nbCriteria_));

	double choquet(0.0);

	// Go first through each interaction
	for (std::size_t i = 0; i + 1 < nbCriteria_; i++)
	{
		for (auto j = i + 1; j < nbCriteria_; j++)
		{
			if (params_(i,j) > 0)
				choquet += params_(i,j) * fmin(values[i], values[j]);
			else
				choquet += fabs(params_(i,j)) * fmax(values[i], values[j]);
		}
	}

	// Go through each criterion
	for (std::size_t i = 0; i < nbCriteria_; i++)
	{
		double tmp(0);
		for (std::size_t j = 0; j < nbCriteria_; j++)
		{
			if (i != j)
				tmp += fabs(params_(i, j));
		}
		choquet += values[i] * (params_(i,i) - (0.5 * tmp));
	}

	return choquet;
}
