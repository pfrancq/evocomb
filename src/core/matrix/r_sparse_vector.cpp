/*

	EvoComb

	RSparseVector.cpp

	Sparse Vector - Implementation.

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


//------------------------------------------------------------------------------
// include files for EvoComb
#include "matrix/r_sparse_vector.hpp"
using namespace evocomb;



//------------------------------------------------------------------------------
//
// class RSparseVector
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
std::vector<RSparseValue>::const_iterator RSparseVector::lowerBound(const std::size_t i) const
{
	return std::lower_bound(values_.begin(), values_.end(), i, [](const RSparseValue& value, const std::size_t id) {
		return value.id_ < id;
	});
}


//------------------------------------------------------------------------------
void RSparseVector::setExisting(const double val)
{
	for (auto&[_, value_] : values_)
		value_ = val;
}


//------------------------------------------------------------------------------
const RSparseValue* RSparseVector::find(const std::size_t i) const
{
	const auto it = lowerBound(i);
	return (it != values_.end()) && it->id_ == i ? &(*it) : nullptr;
}


//------------------------------------------------------------------------------
double RSparseVector::operator[](const std::size_t i) const
{
	const auto* value = find(i);
	return value ? value->value_ : 0.0;
}


//------------------------------------------------------------------------------
double& RSparseVector::operator[](const std::size_t i)
{
	const auto pos = lowerBound(i) - values_.begin();
	if (pos == static_cast<std::ptrdiff_t>(values_.size()) || values_[static_cast<std::size_t>(pos)].id_ != i)
		values_.insert(values_.begin() + pos, RSparseValue{i, 0.0});
	return values_[static_cast<std::size_t>(pos)].value_;
}


//------------------------------------------------------------------------------
bool RSparseVector::remove(const std::size_t i)
{
	const auto it = lowerBound(i);
	if (it == values_.end() || it->id_ != i)
		return false;
	values_.erase(it);
	return true;
}


//------------------------------------------------------------------------------
void RSparseVector::truncate(const std::size_t limit)
{
	values_.erase(lowerBound(limit), values_.end());
}


//------------------------------------------------------------------------------
void RSparseVector::removeNulls()
{
	values_.erase(std::remove_if(values_.begin(), values_.end(), [](const RSparseValue& value) {
		return value.value_ == 0.0;
	}), values_.end());
}
