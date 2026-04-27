/*

	EvoComb

	RMaxVector.cpp

	Sparse Vector Ordered by Decreasing Values - Implementation.

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
#include <algorithm>
#include <numeric>
#include <stdexcept>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "matrix/r_max_vector.hpp"
using namespace evocomb;



//------------------------------------------------------------------------------
//
// class RMaxVector
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
RMaxVector::RMaxVector(const std::size_t maxSize, const std::size_t id)
	: maxSize_(maxSize), id_(id)
{
	if (maxSize_)
		values_.reserve(maxSize_);
}


//------------------------------------------------------------------------------
void RMaxVector::setMaxSize(const std::size_t maxSize)
{
	maxSize_ = maxSize;
	if (maxSize_ && values_.size() > maxSize_)
		values_.resize(maxSize_);
}


//------------------------------------------------------------------------------
std::size_t RMaxVector::add(const std::size_t id, const double value)
{
	// If the identifier is already in the vector, remove it
	if (const auto pos = position(id); pos != cNoRef)
		values_.erase(values_.begin() + static_cast<std::ptrdiff_t>(pos));

	// Position: after the values greater or equal to the new one
	const auto where = std::upper_bound(values_.begin(), values_.end(), value, [](const double v, const RMaxValue& item) {
		return v > item.value_;
	});

	// If the vector is full, the smallest value is removed (if the new value would be the last one, it is not
	// inserted).
	if (maxSize_ && (values_.size() >= maxSize_))
	{
		if (where == values_.end())
			return cNoRef;
		const auto idx = where - values_.begin();
		values_.pop_back();
		values_.insert(values_.begin() + idx, RMaxValue{id, value});
		return static_cast<std::size_t>(idx);
	}

	const auto idx = where - values_.begin();
	values_.insert(where, RMaxValue{id, value});
	return static_cast<std::size_t>(idx);
}


//------------------------------------------------------------------------------
bool RMaxVector::remove(const std::size_t id)
{
	const auto pos = position(id);
	if (pos == cNoRef)
		return false;
	values_.erase(values_.begin() + static_cast<std::ptrdiff_t>(pos));
	return true;
}


//------------------------------------------------------------------------------
std::size_t RMaxVector::position(const std::size_t id) const
{
	const auto it = std::find_if(values_.begin(), values_.end(), [id](const RMaxValue& item) {
		return item.id_ == id;
	});
	return it == values_.end() ? cNoRef : static_cast<std::size_t>(it - values_.begin());
}


//------------------------------------------------------------------------------
const RMaxValue* RMaxVector::find(const std::size_t id) const
{
	const auto pos = position(id);
	return pos == cNoRef ? nullptr : &values_[pos];
}


//------------------------------------------------------------------------------
double RMaxVector::valueOf(const std::size_t id, const double def) const
{
	const auto item = find(id);
	return item ? item->value_ : def;
}


//------------------------------------------------------------------------------
double RMaxVector::max() const
{
	if (values_.empty())
		throw std::out_of_range("RMaxVector::max() : empty vector");
	return values_.front().value_;
}


//------------------------------------------------------------------------------
double RMaxVector::min() const
{
	if (values_.empty())
		throw std::out_of_range("RMaxVector::min() : empty vector");
	return values_.back().value_;
}


//------------------------------------------------------------------------------
double RMaxVector::sum() const
{
	return std::accumulate(values_.begin(), values_.end(), 0.0, [](const double s, const RMaxValue& item) {
		return s + item.value_;
	});
}


//------------------------------------------------------------------------------
double RMaxVector::mean() const
{
	return values_.empty() ? 0.0 : sum() / static_cast<double>(values_.size());
}


//------------------------------------------------------------------------------
std::size_t RMaxVector::countAtLeast(const double threshold) const
{
	// The values are ordered by decreasing order
	const auto it = std::partition_point(values_.begin(), values_.end(), [threshold](const RMaxValue& item) {
		return item.value_ >= threshold;
	});
	return static_cast<std::size_t>(it - values_.begin());
}


//------------------------------------------------------------------------------
bool RMaxVector::operator==(const RMaxVector& vector) const
{
	return id_ == vector.id_ && maxSize_ == vector.maxSize_ && values_ == vector.values_;
}
