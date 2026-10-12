/*

	EvoComb

	RStd.hpp

	R Standard Library - Header.

	Copyright 1999-2026 by Pascal Francq (pascal@francq.info).
	Copyright 1999-2008 by the Université Libre de Bruxelles (ULB).

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
#define __STDC_LIMIT_MACROS
#include <cstddef>
#include <cstdint>
#include <limits>
#include <cmath>
#include <iostream>


//------------------------------------------------------------------------------
// Define __PRETTY_FUNCTION__ if necessary
#ifndef __GNUC__
	#define __PRETTY_FUNCTION__ __FUNCTION__
#endif


//------------------------------------------------------------------------------
// Assertion macros
#ifdef __GNUC__
	#define mInfo \
		"file "<<__FILE__<<": line "<<__LINE__<<" ("<<__PRETTY_FUNCTION__<<")"
#else 	// !__GNUC__
	#define mInfo \
		"file "<<__FILE__<<": line "<<__LINE__
#endif 	// __GNUC__


#ifdef __RDISABLEASSERT__

	#define mAssert(expr)
	#define mAssert2(expr)
	#define mAssertNotReached()

#else 		// !__RDISABLEASSERT__

	#define mAssert(expr)                                             \
	if(!(expr))                                                       \
	{                                                                 \
		std::cerr<<mInfo                                               \
					<<": assertion failed: "<<#expr<<std::endl;           \
	}

	#define mAssert2(expr,msg)                                        \
	if(!(expr))                                                       \
	{                                                                 \
		std::cerr<<mInfo                                               \
					<<": assertion failed: "<<#expr                       \
					<<" ("<<msg<<")"<<std::endl;                          \
	}

	#define mAssertNotReached()                                       \
	{                                                                 \
		std::cerr<<mInfo                                               \
					<<": should not be reached"<<std::endl;               \
	}

	#define mAssertMsg(msg)                                           \
	{                                                                 \
		std::cerr<<mInfo                                               \
					<<": "<<#msg<<std::endl;                              \
	}

#endif 		// !__RDISABLEASSERT__


//------------------------------------------------------------------------------
// Check macros
#ifdef __RDISABLECHECKS__

	#define mReturnIfFail(expr)
	#define mReturnIfFail2(expr,msg)
	#define mReturnValIfFail(expr,val)
	#define mReturnValIfFail3(expr,val,msg)

#else 		// !__RDISABLECHECKS__

	#define mReturnIfFail(expr)                                       \
	if(!(expr))                                                       \
	{                                                                 \
		std::cerr<<mInfo                                               \
					<<": check failed: "<<#expr<<std::endl;               \
		return;                                                        \
	}

	#define mReturnIfFail2(expr,msg)                                  \
	if(!(expr))                                                       \
	{                                                                 \
		std::cerr<<mInfo                                               \
					<<": check failed: "<<#expr                           \
					<<" ("<<msg<<")"<<std::endl;                          \
		return;                                                        \
	}

	#define mReturnValIfFail(expr,val)                                \
	if(!(expr))                                                       \
	{                                                                 \
		std::cerr<<mInfo                                               \
					<<": check failed: "<<#expr<<std::endl;               \
		return(val);                                                   \
	}

	#define mReturnValIfFail3(expr,val,msg)                           \
	if(!(expr))                                                       \
	{                                                                 \
		std::cerr<<mInfo                                               \
					<<": check failed: "<<#expr                           \
					<<" ("<<msg<<")"<<std::endl;                          \
		return(val);                                                   \
	}

#endif 		// !__RDISABLECHECKS__


//------------------------------------------------------------------------------
// To-Do Macros
#ifdef __RDISABLECHECKS__

	#define mToImplement()
	#define mToDo(expr)

#else 		// !__RDISABLECHECKS__

	#define mToImplement()                                             \
	std::cout<<mInfo                                                   \
				<<": to implement"<<std::endl;                            \

	#define mToDo(expr)                                                \
	std::cout<<mInfo                                                   \
				<<") to do: "<<#expr<<std::endl;                          \

#endif 		// !__RDISABLECHECKS__


//-----------------------------------------------------------------------------
// MAX SIZE of size_t
#ifndef SIZE_MAX
	#define SIZE_MAX std::numeric_limits<std::size_t>::max()
#endif



//------------------------------------------------------------------------------
namespace evocomb{
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
* The cNoRef constant represented an undefined identifier reference (or index). It has the type std::size_t, the type of
* the identifiers and of the indexes, so that it can be mixed with them without conversion (for example in the return
* statements of a lambda).
*/
constexpr std::size_t cNoRef=std::numeric_limits<std::size_t>::max();


//------------------------------------------------------------------------------
/**
* The tCoord type represents a coordinate.
*/
typedef double tCoord;


//------------------------------------------------------------------------------
/**
* The cNoCoord constant represent an invalid coordinate. It practice, it is the maximal positive value that a coordinate
* can have.
*/
constexpr tCoord cNoCoord = std::numeric_limits<tCoord>::max();


//------------------------------------------------------------------------------
/**
 * The cMaxCoord represent the maximal acceptable value for a coordinate.
 */
constexpr tCoord cMaxCoord = std::numeric_limits<tCoord>::max();


//------------------------------------------------------------------------------
/**
 * The cMinCoord represent the minimal acceptable value for a coordinate.
 */
constexpr tCoord cMinCoord = std::numeric_limits<tCoord>::min();


//------------------------------------------------------------------------------
/**
* Represent the different orientation of a given polygon.
*/
enum class tOrientation
{
	oNormal,                 /** Normal. */
	oNormalX,                /** Horizontal mirror. */
	oNormalY,                /** Vertical mirror. */
	oNormalYX,               /** Horizontal and vertical mirrors. */
	oRota90,                 /** 90° rotation. */
	oRota90X,                /** 90° rotation with a horizontal mirror. */
	oRota90Y,                /** 90° rotation with a vertical mirror. */
	oRota90YX                /** 90° rotation with a horizontal and vertical
	                             mirror. */
};


//------------------------------------------------------------------------------
/**
* Represent different directions.
*/
enum class tDirection
{
	dNoDirection,            /** No direction. */
	dLeft,                   /** Left. */
	dRight,                  /** Right. */
	dDown,                   /** Down. */
	dUp,                     /** Up. */
	dBehind,                 /** Behind. */
	dBeyond,                 /** Beyond.*/
	dOrigin,                 /** Origin.*/
	dDestination,            /** Specific destination.*/
	dBetween,                /** Between two positions.*/
	dLeftDown,               /** Left-Down.*/
	dLeftUp,                 /** Left-Up.*/
	dRightDown,              /** Right-Down.*/
	dRightUp	                /** Right-Up.*/
};


//------------------------------------------------------------------------------
/**
* This variable represents a tolerance for calculations. if a calculation (for example the difference between two
* coordinates) is less than cEpsi, it is considered as zero.
*/
constexpr double cEpsi=0.000001;


//------------------------------------------------------------------------------
/**
 * The function compares two identifiers. Useful for R::RContainer.
 * @param id1                First identifier.
 * @param id2                Second identifier.
 * @return
 */
inline int CompareIds(const uint64_t id1, const uint64_t id2)
{
	if(id1>id2)
		return 1;
	if(id1<id2)
		return-1;
	return 0;
}


//------------------------------------------------------------------------------
/**
 * The function compares two doubles. Useful for R::RContainer.
 * @param num1               First double.
 * @param num2               Second double.
 * @return
 * - <0 if num1<num2
 * - 0  if num1=num2
 * - >0 if num1>num2
 */
inline int CompareDoubles(const double num1, const double num2)
{
	if (std::abs(num1 - num2) <= cEpsi)
		return 0;
	if (num1 > num2)
		return 1.0;
	return -1.0;
}


//------------------------------------------------------------------------------
/**
 * The function compares two pointers. Useful for R::RContainer.
 * @tparam C                 Class to compare.
 * @param ptr1               First pointer.
 * @param ptr2               Second pointer.
 */
template<class C>
int ComparePtrs(C* ptr1, C* ptr2)
{
	if (ptr1 > ptr2)
		return 1;
	if (ptr1 < ptr2)
		return -1;
	return 0;
}


}  //-------- End of namespace evocomb ---------------------------------------------
