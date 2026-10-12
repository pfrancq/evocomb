/*

	EvoComb

	RPromKernel.cpp

	PROMETHEE Kernel - Implementation.

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
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <vector>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "multicriteria/r_prom_kernel.hpp"
#include "multicriteria/r_prom_crit_value.hpp"
#include "tools/r_exception.hpp"
using namespace evocomb;


//------------------------------------------------------------------------------
mLogModule("r.promethee");



//------------------------------------------------------------------------------
//
// class RPromKernel
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
RPromKernel::RPromKernel(RLogger& logger, std::string name):
	logger_(logger),
	name_(std::move(name))
{
}


//------------------------------------------------------------------------------
void RPromKernel::computePrometheeII()
{
	mLogBlock(logger_, "computePrometheeII");

	double sumWTot = 0.0;

	// Calculation of Fit Crit for the criteria.
	for (const auto& crit: criteria_)
	{
		sumWTot += crit->weight_;
		crit->computeFiCrit();
	}

	// Calculation of the flow.
	for (const auto& sol: solutions_)
	{
		sol->fiPlus_ = sol->fiMinus_ = 0.0;
		for (const auto& criteria: criteria_)
		{
			const auto& value = criteria->values_[sol->index()];
			sol->fiPlus_ += criteria->weight_ * value.fiPlus_;
			sol->fiMinus_ += criteria->weight_ * value.fiMinus_;
		}
		// Each solution is compared with the (n-1) other ones
		if (solutions_.size() > 1)
		{
			sol->fiPlus_ /= sumWTot * static_cast<double>(solutions_.size() - 1);
			sol->fiMinus_ /= sumWTot * static_cast<double>(solutions_.size() - 1);
		}
		sol->fi_ = sol->fiPlus_ - sol->fiMinus_;
		mLogDebug(logger_, sol->name_, ": ϕ+=", evocomb::RConcat::fixed(sol->fiPlus_, 3), " ; ϕ-=", evocomb::RConcat::fixed(sol->fiMinus_, 3), " ; ϕ=", evocomb::RConcat::fixed(sol->fi_, 3));
	}

	mustReOrder_ = true;
}


//------------------------------------------------------------------------------
void RPromKernel::addCriterion(RPromCriterion::Ptr&& crit)
{
	mLogDebug(logger_, "Adding criterion: ", crit->name());
	crit->index_ = criteria_.size();
	criteria_.emplace_back(std::move(crit));
}


//------------------------------------------------------------------------------
void RPromKernel::addCriteria(const RJSON& criteria)
{
	if (!criteria.isArray())
		mThrowRException("PROMETHEE criteria must be a JSON array");

	// Read a number (integer or decimal) with a default value
	const auto readNumber = [](const RJSON& crit, const std::string& key, const double def) {
		const auto param = crit.find(key);
		if (param == nullptr)
			return def;
		if (param->isDecimal())
			return param->toDecimal();
		if (param->isInteger())
			return static_cast<double>(param->toInteger());
		mThrowRException("PROMETHEE criterion: '", key, "' must be a number");
	};

	// Build all the criteria before adding them
	std::vector<RPromCriterion::Ptr> newCriteria;
	for (size_t i = 0; ; ++i)
	{
		const auto crit = criteria.find(std::to_string(i));
		if (crit == nullptr)
			break;
		if (!crit->isObject())
			mThrowRException("PROMETHEE criterion ", i, " must be a JSON object");

		std::string name;
		if (const auto param = crit->find("name"); param != nullptr)
		{
			if (!param->isString())
				mThrowRException("PROMETHEE criterion ", i, ": 'name' must be a string");
			name = param->toString();
		}

		const auto typeParam = crit->find("type");
		if ((typeParam == nullptr) || (!typeParam->isString()))
			mThrowRException("PROMETHEE criterion ", i, ": 'type' (\"minimize\" or \"maximize\") is mandatory");
		RCriteriaType type;
		if (typeParam->toString() == "minimize")
			type = RCriteriaType::Minimize;
		else if (typeParam->toString() == "maximize")
			type = RCriteriaType::Maximize;
		else
			mThrowRException("PROMETHEE criterion ", i, ": invalid type '", typeParam->toString(), "'");

		const auto p = readNumber(*crit, "p", 0.2);
		const auto q = readNumber(*crit, "q", 0.05);
		const auto weight = readNumber(*crit, "weight", 1.0);
		if ((q < 0.0) || (p < q))
			mThrowRException("PROMETHEE criterion ", i, ": thresholds must verify 0 <= q <= p");
		if (weight <= 0.0)
			mThrowRException("PROMETHEE criterion ", i, ": 'weight' must be strictly positive");
		newCriteria.push_back(std::make_unique<RPromLinearCriterion>(type, p, q, weight, name));
	}

	for (auto& crit: newCriteria)
		addCriterion(std::move(crit));
}


//------------------------------------------------------------------------------
RPromCriterion& RPromKernel::criterion(const std::string& name) const
{
	for (auto& crit: criteria_)
		if (crit->name_ == name)
			return *crit;
	mThrowRException("Missing criterion");
}


//------------------------------------------------------------------------------
RPromCriterion& RPromKernel::criterion(const uint64_t id) const
{
	for (auto& crit: criteria_)
		if (crit->index_ == id)
			return *crit;
	mThrowRException("Missing criterion");
}


//------------------------------------------------------------------------------
void RPromKernel::createSolutions(const std::vector<std::string>& names)
{
	solutions_.reserve(names.size() + solutions_.size());
	for (const auto& name: names)
	{
		mLogDebug(logger_, "Adding solution: ", name);
		auto sol = std::make_unique<RPromSol>(name);
		sol->index_ = solutions_.size();
		solutions_.emplace_back(std::move(sol));
	}
}


//------------------------------------------------------------------------------
void RPromKernel::addSolution(RPromSol::Ptr&& solution)
{
	mLogDebug(logger_, "Adding solution: ", solution->name());
	solution->index_ = solutions_.size();
	solutions_.emplace_back(std::move(solution));
}


//------------------------------------------------------------------------------
void RPromKernel::assign(const uint64_t solIdx, const uint64_t critIdx, const double v)
{
	if (solutions_.size() < solIdx || criteria_.size() < critIdx)
		mThrowRException("Invalid assignment");
	for (const auto& crit: criteria_)
		crit->values_.resize(solutions_.size());
	criteria_[critIdx]->values_[solIdx].value_ = v;
}


//------------------------------------------------------------------------------
void RPromKernel::assign(const uint64_t solIdx, const std::vector<double>& values)
{
	if (solutions_.size() < solIdx || criteria_.size() != values.size())
		mThrowRException("Invalid assignment");

	for (const auto& crit: criteria_)
		crit->values_.resize(solutions_.size());

	auto critIdx = 0u;
	for (auto& value: values)
	{
		criteria_[critIdx]->values_[solIdx].value_ = value;
		critIdx++;
	}
}


//------------------------------------------------------------------------------
const RPromSol& RPromKernel::bestSolution() const
{
	if (solutions_.empty())
		mThrowRException("No solution");
	if (mustReOrder_)
		orderSolutions();
	return *orderedSolutions_.front();
}


//------------------------------------------------------------------------------
const std::vector<RPromSol*>& RPromKernel::solutions() const
{
	if (mustReOrder_)
		orderSolutions();
	return orderedSolutions_;
}


//------------------------------------------------------------------------------
double RPromKernel::minFi() const
{
	if (solutions_.empty())
		return 0.0;
	if (mustReOrder_)
		orderSolutions();
	return orderedSolutions_.back()->fi_;
}


//------------------------------------------------------------------------------
double RPromKernel::maxFi() const
{
	if (solutions_.empty())
		return 0.0;
	if (mustReOrder_)
		orderSolutions();
	return orderedSolutions_.front()->fi_;
}


//------------------------------------------------------------------------------
void RPromKernel::clear()
{
	criteria_.clear();
	solutions_.clear();
	orderedSolutions_.clear();
}


//------------------------------------------------------------------------------
void RPromKernel::clearSols()
{
	for (const auto& criteria: criteria_)
		criteria->values_.clear();
	solutions_.clear();
	orderedSolutions_.clear();
}


//------------------------------------------------------------------------------
void RPromKernel::print() const
{
	if (solutions_.empty())
		return;
	if (mustReOrder_)
		orderSolutions();

	// Print Header
	std::cout << std::setw(7) << std::left << "Index";
	std::cout << std::setw(15) << std::left << "Fi";
	std::cout << std::setw(15) << std::left << "Fi+";
	std::cout << std::setw(15) << std::left << "Fi-";
	for (const auto& criterion: criteria_)
		std::cout << std::setw(20) << std::left << criterion->name();
	std::cout << std::setw(15) << std::left << "Name" << std::endl;

	// Print Solution
	for (const auto sol: orderedSolutions_)
	{
		std::cout<<std::setw(7) << std::left << sol->index_;
		std::cout<<std::setw(15) << std::left << sol->fi();
		std::cout<<std::setw(15) << std::left << sol->fiPlus();
		std::cout<<std::setw(15) << std::left << sol->fiMinus();
		for (const auto& crit: criteria_)
			std::cout << std::setw(20) << std::left << crit->values_[sol->index_].value_;
		std::cout << std::setw(15) << std::left << sol->name_.substr(0, 15) << std::endl;
	}
}


//------------------------------------------------------------------------------
void RPromKernel::orderSolutions() const
{
	orderedSolutions_.clear();
	orderedSolutions_.reserve(solutions_.size());
	for (auto& sol: solutions_)
		orderedSolutions_.emplace_back(sol.get());
	std::sort(orderedSolutions_.begin(), orderedSolutions_.end(), [](const RPromSol* a, const RPromSol* b) {
		if (!a || !b)
			return a > b;
		return a->fi() > b->fi();
	});
	mustReOrder_ = false;
}
