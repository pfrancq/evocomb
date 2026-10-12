/*

	EvoComb

	RSparseVector.hpp

	Sparse Vector - Header.

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
#pragma once


//------------------------------------------------------------------------------
// include files for ANSI C/C++
#include <cstddef>
#include <vector>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "tools/r_std.hpp"


//------------------------------------------------------------------------------
namespace evocomb{
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
 * The RSparseValue structure associates a value to an index (see RSparseVector).
 * @short Indexed Value.
 */
struct RSparseValue
{
	/**
	 * Compare two indexed values.
	 * @param other				Indexed value.
	 */
	[[nodiscard]] bool operator==(const RSparseValue& other) const { return id_ == other.id_ && value_ == other.value_; }

	/**
	 * Compare two indexed values.
	 * @param other				Indexed value.
	 */
	[[nodiscard]] bool operator!=(const RSparseValue& other) const { return !(*this == other); }

	//-----------------------------------------------------
	std::size_t id_ = cNoRef;					///< Index.
	double value_ = 0.0;						///< Value.
};


//------------------------------------------------------------------------------
/**
 * The RSparseVector class provides a sparse vector: only the elements that were set are stored (ordered by increasing
 * indexes), the other ones are null. The vector itself can have an identifier (used by RSparseMatrix).
 * @code
 * evocomb::RSparseVector a;
 * a.set(0, 1.0);
 * a[5] = 2.0;                                      // Creates the element 5.
 * a.set(15, 3.0);
 * std::cout << std::as_const(a)[3] << std::endl;   // 0 (nothing created).
 * for (const auto& [id, value] : a)
 *     std::cout << id << ": " << value << std::endl;
 * @endcode
 * As for std::map, the non-const operator[] creates a null element if the index has no value: to only read, use a const
 * reference (or std::as_const(a)[i]). The reference returned by the non-const operator[] is invalidated when another
 * element is created or removed.
 * @short Sparse Vector.
 */
class RSparseVector
{
public:

	/**
	 * Construct an empty vector.
	 * @param id				Identifier of the vector.
	 */
	explicit RSparseVector(std::size_t id = cNoRef) : id_(id) {}

	/**
	 * Get the identifier of the vector.
	 */
	[[nodiscard]] std::size_t id() const { return id_; }

	/**
	 * Get the number of elements stored.
	 */
	[[nodiscard]] std::size_t size() const { return values_.size(); }

	/**
	 * Look if the vector has no element stored.
	 */
	[[nodiscard]] bool empty() const { return values_.empty(); }

	/**
	 * Reserve the memory for a given number of elements.
	 * @param nb				Number of elements.
	 */
	void reserve(const std::size_t nb) { values_.reserve(nb); }

	/**
	 * Remove all the elements.
	 */
	void clear() { values_.clear(); }

	/**
	 * Set a given value to all the existing elements.
	 * @param val				Value.
	 */
	void setExisting(double val);

	/**
	 * Look if an index has an element in the vector.
	 * @param i					Index.
	 */
	[[nodiscard]] bool contains(const std::size_t i) const { return find(i) != nullptr; }

	/**
	 * Get the element at a given index.
	 * @param i					Index.
	 * @return a pointer to the element, or nullptr if the index has no element.
	 */
	[[nodiscard]] const RSparseValue* find(std::size_t i) const;

	/**
	 * Get the value at a given index (0 if the index has no element).
	 * @param i					Index.
	 */
	[[nodiscard]] double operator[](std::size_t i) const;

	/**
	 * Get the value at a given index. If the index has no element, a null element is created.
	 * @param i					Index.
	 */
	[[nodiscard]] double& operator[](std::size_t i);

	/**
	 * Set the value at a given index (the element is created if necessary).
	 * @param i					Index.
	 * @param val				Value.
	 */
	void set(const std::size_t i, const double val) { (*this)[i] = val; }

	/**
	 * Add a value to the element at a given index (the element is created if necessary).
	 * @param i					Index.
	 * @param val				Value.
	 */
	void add(const std::size_t i, const double val) { (*this)[i] += val; }

	/**
	 * Remove the element at a given index.
	 * @param i					Index.
	 * @return `true` if the index had an element.
	 */
	bool remove(std::size_t i);

	/**
	 * Remove all the elements whose index is greater or equal to a limit.
	 * @param limit				Limit.
	 */
	void truncate(std::size_t limit);

	/**
	 * Remove all the null elements.
	 */
	void removeNulls();

	/**
	 * Get the elements (ordered by increasing indexes).
	 */
	[[nodiscard]] const std::vector<RSparseValue>& values() const { return values_; }

	// Iterators (needed for range-based for loops)
	[[nodiscard]] auto begin() const { return values_.begin(); }
	[[nodiscard]] auto end() const { return values_.end(); }

	/**
	 * Compare two vectors (identifiers and elements).
	 * @param vector			Vector.
	 */
	[[nodiscard]] bool operator==(const RSparseVector& vector) const { return id_ == vector.id_ && values_ == vector.values_; }

	/**
	 * Compare two vectors.
	 * @param vector			Vector.
	 */
	[[nodiscard]] bool operator!=(const RSparseVector& vector) const { return !(*this == vector); }

private:

	/**
	 * Get the position of the first element whose index is not smaller than a given one.
	 * @param i					Index.
	 */
	[[nodiscard]] std::vector<RSparseValue>::const_iterator lowerBound(std::size_t i) const;

	//-----------------------------------------------------
	std::vector<RSparseValue> values_;		///< Elements (by increasing indexes).
	std::size_t id_;						///< Identifier of the vector.

	//-----------------------------------------------------
	friend class RSparseMatrix;
	friend class RMatrixStorage;
};


}  //-------- End of namespace evocomb ---------------------------------------------
