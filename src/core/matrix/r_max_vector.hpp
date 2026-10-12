/*

	EvoComb

	RMaxVector.hpp

	Sparse Vector Ordered by Decreasing Values - Header.

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
 * The RMaxValue structure associates a value to an identifier (see RMaxVector).
 * @short Identified Value.
 */
struct RMaxValue
{
	/**
	 * Compare two identified values.
	 * @param other				Identified value.
	 */
	[[nodiscard]] bool operator==(const RMaxValue& other) const { return id_ == other.id_ && value_ == other.value_; }

	/**
	 * Compare two identified values.
	 * @param other				Identified value.
	 */
	[[nodiscard]] bool operator!=(const RMaxValue& other) const { return !(*this == other); }

	//-----------------------------------------------------
	std::size_t id_ = cNoRef;				///< Identifier.
	double value_ = 0.0;					///< Value.
};


//------------------------------------------------------------------------------
/**
 * The RMaxVector class provides a sparse vector whose values are ordered by decreasing order (the maximal value is at
 * position 0). Each value is associated to an identifier (for example the identifier of an object), and an identifier
 * appears at most once. It is typically used to store, for an object, its nearest neighbors ordered by decreasing
 * similarity.
 *
 * The vector can have a maximal size: when it is full, a new value is only inserted if it is greater than the smallest
 * one, which is then removed (the vector keeps the "n best" values).
 *
 * The vector itself can have an identifier (used by RMaxMatrix).
 * @code
 * evocomb::RMaxVector a;
 * a.add(1, 3.0);          // Identifier 1 is associated with value 3.0.
 * a.add(2, 5.0);          // Identifier 2 is associated with value 5.0.
 * a.add(3, 1.0);          // Identifier 3 is associated with value 1.0.
 * for (const auto& [id, value] : a)
 *     std::cout << id << ": " << value << std::endl;   // 2: 5, 1: 3, 3: 1
 * @endcode
 * @short Sparse Vector Ordered by Decreasing Values.
 */
class RMaxVector
{
public:

	/**
	 * Construct an empty vector.
	 * @param maxSize			Maximal number of values (0 means no limit).
	 * @param id				Identifier of the vector.
	 */
	explicit RMaxVector(std::size_t maxSize = 0, std::size_t id = cNoRef);

	/**
	 * Get the identifier of the vector.
	 */
	[[nodiscard]] std::size_t id() const { return id_; }

	/**
	 * Get the maximal number of values (0 means no limit).
	 */
	[[nodiscard]] std::size_t maxSize() const { return maxSize_; }

	/**
	 * Set the maximal number of values. If the vector contains more values, the smallest ones are removed.
	 * @param maxSize			Maximal number of values (0 means no limit).
	 */
	void setMaxSize(std::size_t maxSize);

	/**
	 * Get the number of values.
	 */
	[[nodiscard]] std::size_t size() const { return values_.size(); }

	/**
	 * Look if the vector is empty.
	 */
	[[nodiscard]] bool empty() const { return values_.empty(); }

	/**
	 * Remove all the values.
	 */
	void clear() { values_.clear(); }

	/**
	 * Add a value associated to an identifier. If the identifier is already in the vector, its value is replaced (and
	 * its position updated). A value is inserted after the values that are equal to it.
	 * @param id				Identifier.
	 * @param value				Value.
	 * @return the position of the value, or cNoRef if the vector is full and the value is smaller or equal to all the
	 * values of the vector.
	 */
	std::size_t add(std::size_t id, double value);

	/**
	 * Remove the value associated to an identifier.
	 * @param id				Identifier.
	 * @return `true` if the identifier was in the vector.
	 */
	bool remove(std::size_t id);

	/**
	 * Look if an identifier has a value in the vector.
	 * @param id				Identifier.
	 */
	[[nodiscard]] bool contains(const std::size_t id) const { return position(id) != cNoRef; }

	/**
	 * Get the position of the value associated to an identifier.
	 * @param id				Identifier.
	 * @return the position or cNoRef if the identifier has no value.
	 */
	[[nodiscard]] std::size_t position(std::size_t id) const;

	/**
	 * Get the value associated to an identifier.
	 * @param id				Identifier.
	 * @return a pointer to the identified value, or nullptr if the identifier has no value.
	 */
	[[nodiscard]] const RMaxValue* find(std::size_t id) const;

	/**
	 * Get the value associated to an identifier.
	 * @param id				Identifier.
	 * @param def				Value returned if the identifier has no value.
	 */
	[[nodiscard]] double valueOf(std::size_t id, double def = 0.0) const;

	/**
	 * Get the identified value at a given position (0 is the maximal value).
	 * @param pos				Position.
	 */
	[[nodiscard]] const RMaxValue& operator[](const std::size_t pos) const { return values_[pos]; }

	/**
	 * Get the value at a given position (0 is the maximal value).
	 * @param pos				Position.
	 */
	[[nodiscard]] double value(const std::size_t pos) const { return values_[pos].value_; }

	/**
	 * Get the maximal value.
	 * @exception std::out_of_range The vector is empty.
	 */
	[[nodiscard]] double max() const;

	/**
	 * Get the minimal value.
	 * @exception std::out_of_range The vector is empty.
	 */
	[[nodiscard]] double min() const;

	/**
	 * Compute the sum of the values.
	 */
	[[nodiscard]] double sum() const;

	/**
	 * Compute the mean of the values (0 if the vector is empty).
	 */
	[[nodiscard]] double mean() const;

	/**
	 * Count the values greater or equal to a threshold (they are at the beginning of the vector).
	 * @param threshold			Threshold.
	 */
	[[nodiscard]] std::size_t countAtLeast(double threshold) const;

	/**
	 * Get the identified values (by decreasing values).
	 */
	[[nodiscard]] const std::vector<RMaxValue>& values() const { return values_; }

	// Iterators (needed for range-based for loops)
	[[nodiscard]] auto begin() const { return values_.begin(); }
	[[nodiscard]] auto end() const { return values_.end(); }

	/**
	 * Compare two vectors (identifiers, maximal sizes and values).
	 * @param vector			Vector.
	 */
	[[nodiscard]] bool operator==(const RMaxVector& vector) const;

	/**
	 * Compare two vectors.
	 * @param vector			Vector.
	 */
	[[nodiscard]] bool operator!=(const RMaxVector& vector) const { return !(*this == vector); }

private:

	//-----------------------------------------------------
	std::vector<RMaxValue> values_;			///< Values (by decreasing order).
	std::size_t maxSize_;					///< Maximal number of values (0 = no limit).
	std::size_t id_;						///< Identifier of the vector.

	//-----------------------------------------------------
	friend class RMaxMatrix;
	friend class RMatrixStorage;
};


}  //-------- End of namespace evocomb ---------------------------------------------
