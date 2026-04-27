/*

	EvoComb

	RPromCriterion.cpp

	PROMETHEE Criterion - Implementation.

	Copyright 2000-2026 by Pascal Francq (pascal@francq.info).
	Copyright 2000-2008 by the Université Libre de Bruxelles (ULB).

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
#include <cmath>
#include <utility>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "multicriteria/r_prom_criterion.hpp"
#include "multicriteria/r_prom_crit_value.hpp"
#include "multicriteria/r_prom_kernel.hpp"
#include "tools/r_exception.hpp"
#include "tools/r_std.hpp"
using namespace evocomb;



//------------------------------------------------------------------------------
//
// class RPromCriterion
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
RPromCriterion::RPromCriterion(const RCriteriaType type,const double weight, std::string name):
	index_(cNoRef),
	name_(std::move(name)),
	type_(type),
	weight_(weight),
	isActive_(true)
{
}


//------------------------------------------------------------------------------
void RPromCriterion::computeFiCrit()
{
	// Calculation of Fi Crit + & -
	for (auto& item1: values_)
	{
		item1.fiPlus_ = item1.fiMinus_ = 0.0;
		for (const auto& item2: values_)
		{
			if (&item1 == &item2)
				continue;
			item1.fiPlus_ += computePref(item1.value_, item2.value_);
			item1.fiMinus_ += computePref(item2.value_, item1.value_);
		}
	}

	// Compute Fi Crit
	for (auto& item: values_)
		item.fi_ = item.fiPlus_ - item.fiMinus_;
}



//------------------------------------------------------------------------------
//
// class RPromLinearCriterion
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
RPromLinearCriterion::RPromLinearCriterion(const RCriteriaType type, const double p, const double q, const double weight, std::string name):
	RPromCriterion(type, weight,std::move(name)),
	p_(p),
	q_(q)
{
}


//------------------------------------------------------------------------------
double RPromLinearCriterion::computePref(const double u, const double v)
{
	if (fabs(u - v) < 0.000001)
		return 0.0;

	// Compute the relative difference
	double d;
	if (u > v)
		d = (u - v) / u;
	else
		d = (v - u) / v;

	// No solution is better
	if (d <= q_)
		return 0.0;

	// One solution is better than the other one, but it depends on:
	// If u>v or v<u
	// If the criteria should be maximized or minimized
	if (d >= p_)
	{
		if (type_ == RCriteriaType::Maximize)
		{
			if (u > v)
				return 1.0;
			return 0.0;
		}
		if (u < v)
			return 1.0;
		return 0.0;
	}

	// Between Q and P -> Compute the preference.
	if (type_ == RCriteriaType::Maximize)
	{
		if (u > v)
			return (d - q_) / (p_ - q_);
		return 0;
	}
	if (u < v)
		return (d - q_) / (p_ - q_);
	return 0;
}
