/*

	EvoComb

	RVector.hpp

	Class representing vector - Header

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
#pragma once


//------------------------------------------------------------------------------
// include files for ANSI C/C++
#include <algorithm>
#include <iomanip>
#include <string>
#include <vector>


//------------------------------------------------------------------------------
namespace evocomb{
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
 * The RVector class provides a representation for a vector. It is a specialized class inheriting from RNumContainer
 * since a vector is supposed to have a fixed size.
 * @short RVector
 */
class RVector
{
public:

	/**
	* Construct a vector.
	* @param size            Size of the vector.
	*/
	explicit RVector(size_t size);

	/**
	 * Construct a vector.
	 * @param values		Values to move.
	 */
	explicit RVector(std::vector<double>&& values);

	/**
	 * Destructor.
	 */
	virtual ~RVector() = default;

	/**
	 * Initialize all the elements to a given value.
	 * @param val             Value to assign.
    */
	virtual void init(double val);

	/**
	 * Initialize all the elements with 0.
	 */
	void init() { init(0.0); }

	/**
	 * Resize the vector.
	 * @param newSize		New size.
	 */
	void resize(const size_t newSize) { values_.resize(newSize); }

	/**
	* Resize the size of the vector.
   * @param newSize			New size.
   * @param val				Value to assign to the new elements created.
	*/
	void resize(size_t newSize, double val);

	/**
	* Test if two vectors have exactly the same values.
	* @param vector           Vector used for the comparison.
	* @return True if the vectors are the same.
	*/
	[[nodiscard]] bool operator==(const RVector& vector) const { return values_ == vector.values_; }

	/**
	* Test if two vectors have exactly the same values.
	* @param vector           Vector used for the comparison.
	* @return True if the vectors are the same.
	*/
	[[nodiscard]] bool operator!=(const RVector& vector) const { return values_ != vector.values_; }

	/**
	* Verify if a value is in the vector.
	* @param value            Value to test.
	* @return True if the value is in the vector, false else.
	*/
	[[nodiscard]] bool contains(const double value) const { return std::find(values_.begin(), values_.end(), value) != values_.end(); }

	/**
	* Reorder the vector in ascending order.
	*/
	void reOrder() { std::sort(values_.begin(), values_.end()); }

	/**
	* Randomize the vector.
	* @param rand             Random number generator to use.
	*/
	template<typename R>
		void randomize(R& rand) { std::shuffle(values_.begin(), values_.end(), rand);}

	/**
	 * Get the values of the vector.
	 * @return a const reference to a vector of doubles.
	 */
	[[nodiscard]] const std::vector<double>& values() const { return values_; }

	/**
	 * Get the values of the vector.
	 * @return a reference to a vector of doubles.
	 */
	[[nodiscard]] std::vector<double>& values() { return values_; }

	/**
	* Add a vector to the current one.
	* @param vector          Vector to add.
	*/
	RVector& operator+=(const RVector& vector);

	/**
	* Subtract a vector from the current one.
	* @param vector          Vector to subtract.
	*/
  	RVector& operator-=(const RVector& vector);

	/**
	* Multiply a vector with a given number.
	* @param arg             Number.
	*/
	RVector& operator*=(double arg);

	/**
	* Divide a vector with a given number.
	* @param arg             Number.
	*/
	RVector& operator/=(double arg);

	/**
	 * Product between two vectors.
	 * @param vector		Second vector.
	 * @return a double
	 */
	double operator*(const RVector& vector) const;

	/**
	 * Print the content of the vector.
	 * @tparam S				Stream class that implements the << operator.
	 * @param stream			Stream.
	 * @param name				Name of the vector.
	 * @param precision			Precision.
	 * @param scientific		Scientific notation.
	 * @param colSize			Size of a column. By default, it is 12.
	 */
	template<typename S>
		void print(S& stream, const std::string& name, const int precision = 4, const bool scientific = true, const int colSize=12) const
	{
		stream << name << "=[";
		if (scientific)
			stream << std::scientific;
		stream << std::setprecision(precision);
		for (auto& item: values_)
				stream << std::setw(colSize) << item;
		stream << "]" << std::endl;
	}

	/**
	 * Access element by index (read/write).
	 * @param index Position of the element.
	 * @return Reference to the element.
	 */
	double& operator[](const size_t index) { return values_[index]; }

	/**
	 * Access element by index (read-only).
	 * @param index Position of the element.
	 * @return Const reference to the element.
	 */
	const double& operator[](const size_t index) const { return values_[index]; }

	// Iterators (Needed for range-based for loops)
	auto begin() { return values_.begin(); }
	auto end()   { return values_.end(); }
	[[nodiscard]] auto begin() const { return values_.begin(); }
	[[nodiscard]] auto end()   const { return values_.end(); }

private:

	//-----------------------------------------------------
	std::vector<double> values_;				///< Values

	//-----------------------------------------------------
	friend class RMatrix;
	friend class RLowerTriangularMatrix;
};


//------------------------------------------------------------------------------
/**
* Addition of two vectors.
* @param arg1                Vector.
* @param arg2                Vector.
*/
RVector operator+(const RVector& arg1, const RVector& arg2);


//------------------------------------------------------------------------------
/**
* Subtraction of two vectors.
* @param arg1                Vector.
* @param arg2                Vector.
*/
RVector operator-(const RVector& arg1, const RVector& arg2);


//------------------------------------------------------------------------------
/**
* Multiplication of a vector and a number.
* @param arg1                Vector.
* @param arg2                Vector.
*/
RVector operator*(const RVector& arg1, double arg2);


//------------------------------------------------------------------------------
/**
* Multiplication of a vector and a number.
* @param arg1                Number.
* @param arg2                Vector.
*/
RVector operator*(double arg1, const RVector& arg2);


//------------------------------------------------------------------------------
/**
* Division of a vector and a number.
* @param arg1                Vector.
* @param arg2                Vector.
*/
RVector operator/(const RVector& arg1, double arg2);


//------------------------------------------------------------------------------
/**
* Division of a vector and a number.
* @param arg1                Number.
* @param arg2                Vector.
*/
RVector operator/(double arg1, const RVector& arg2);


//------------------------------------------------------------------------------
/**
* Multiplication of two vectors, i.e. compute the scalar product.
* @param arg1                Vector.
* @param arg2                Vector.
* @return the scalar product.
*/
double operator*(const RVector& arg1, const RVector& arg2);


}  //------- End of namespace evocomb ----------------------------------------------
