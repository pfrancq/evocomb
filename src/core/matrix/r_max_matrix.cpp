/*

	EvoComb

	RMaxMatrix.cpp

	Sparse Matrix of Vectors Ordered by Decreasing Values - Implementation.

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
#include <stdexcept>
#include <string>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "app/r_logger.hpp"
#include "matrix/r_max_matrix.hpp"
#include "tools/r_exception.hpp"
using namespace evocomb;


//------------------------------------------------------------------------------
mLogModule("r.core");



//------------------------------------------------------------------------------
//
// class RMaxMatrix
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
RMaxMatrix::RMaxMatrix(const std::size_t nbLines, const std::size_t nbCols)
	: RGenericMatrix(nbLines, nbCols)
{
	lines_.reserve(nbLines);
	for (std::size_t i = 0; i < nbLines; i++)
		lines_.emplace_back(nbCols, i);
}


//------------------------------------------------------------------------------
void RMaxMatrix::clear(double, bool)
{
	for (auto& line : lines_)
		line.clear();
}


//------------------------------------------------------------------------------
void RMaxMatrix::init(double)
{
	mThrowRException("init is not valid for RMaxMatrix: use add");
}


//------------------------------------------------------------------------------
void RMaxMatrix::verifySize(const std::size_t newLines, const std::size_t newCols, bool, double)
{
	if (newLines < lines_.size())
		lines_.resize(newLines, RMaxVector(newCols));
	else
		for (auto i = lines_.size(); i < newLines; i++)
			lines_.emplace_back(newCols, i);
	for (auto& line : lines_)
		line.setMaxSize(newCols);
	nbLines_ = newLines;
	nbCols_ = newCols;
}


//------------------------------------------------------------------------------
double RMaxMatrix::operator()(const std::size_t i, const std::size_t j) const
{
	if (i >= nbLines_ || (nbCols_ && j >= nbCols_))
		throw std::range_error("RMaxMatrix::operator() const : index " + std::to_string(i) + "," + std::to_string(j) + " outside range (" + std::to_string(nbLines_) + "," + std::to_string(nbCols_) + ")");
	const auto& line = lines_[i];
	return j < line.size() ? line.value(j) : 0.0;
}


//------------------------------------------------------------------------------
double& RMaxMatrix::operator()(std::size_t, std::size_t)
{
	mThrowRException("operator() is not valid for RMaxMatrix: use add");
}


//------------------------------------------------------------------------------
void RMaxMatrix::add(const std::size_t i, const std::size_t id, const double val)
{
	(*this)[i].add(id, val);
}


//------------------------------------------------------------------------------
const RMaxVector& RMaxMatrix::operator[](const std::size_t i) const
{
	if (i >= nbLines_)
		throw std::range_error("RMaxMatrix::operator[] const : index " + std::to_string(i) + " outside range (0," + std::to_string(nbLines_) + ")");
	return lines_[i];
}


//------------------------------------------------------------------------------
RMaxVector& RMaxMatrix::operator[](const std::size_t i)
{
	if (i >= nbLines_)
		throw std::range_error("RMaxMatrix::operator[] : index " + std::to_string(i) + " outside range (0," + std::to_string(nbLines_) + ")");
	return lines_[i];
}
