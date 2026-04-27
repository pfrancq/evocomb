/*

    EvoComb

    RHGAAttrList.hpp

    List of Attributes of the Hierarchical Genetic Algorithm - Header.

    Copyright 2002-2026 by Pascal Francq (pascal@francq.info).
    Copyright 2002-2008 by the Université Libre de Bruxelles (ULB).

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
#include <initializer_list>
#include <string>
#include <vector>


//------------------------------------------------------------------------------
namespace evocomb {
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
 * The RHGAAttrList class provides a list of attributes (identified by numbers) used by the Hierarchical Genetic
 * Algorithm: each object to place in the tree, and each node of the tree, is described by such a list. The attributes
 * are kept ordered and without duplicates, which allows set operations in linear time.
 * @code
 * evocomb::RHGAAttrList a{3, 1, 2};    // {1, 2, 3}
 * evocomb::RHGAAttrList b{1, 2};
 * b.isParentOf(a);                 // true: b is included in a
 * a.nbCommon(b);                   // 2
 * @endcode
 * @short HGA List of Attributes.
 */
class RHGAAttrList
{
public:

    /**
     * Construct an empty list.
     */
    RHGAAttrList() = default;

    /**
     * Construct a list from some attributes (in any order, the duplicates are removed).
     * @param attrs              Attributes.
     */
    RHGAAttrList(std::initializer_list<std::size_t> attrs);

    /**
     * Construct a list from some attributes (in any order, the duplicates are removed).
     * @param attrs              Attributes.
     */
    explicit RHGAAttrList(std::vector<std::size_t> attrs);

    /**
     * Insert an attribute (nothing is done if it is already in the list).
     * @param attr               Attribute.
     */
    void insert(std::size_t attr);

    /**
     * Remove all the attributes.
     */
    void clear() { attrs_.clear(); }

    /**
     * Get the number of attributes.
     */
    [[nodiscard]] std::size_t size() const { return attrs_.size(); }

    /**
     * Look if the list is empty.
     */
    [[nodiscard]] bool empty() const { return attrs_.empty(); }

    /**
     * Get the attribute at a given position.
     * @param pos                Position.
     */
    [[nodiscard]] std::size_t operator[](const std::size_t pos) const { return attrs_[pos]; }

    /**
     * Get the attributes (ordered).
     */
    [[nodiscard]] const std::vector<std::size_t>& values() const { return attrs_; }

    /**
     * Look if an attribute is in the list.
     * @param attr               Attribute.
     */
    [[nodiscard]] bool contains(std::size_t attr) const;

    /**
     * Look if the list is a "parent" of another one, i.e. all its attributes are in the other list.
     * @param child              Other list.
     */
    [[nodiscard]] bool isParentOf(const RHGAAttrList& child) const;

    /**
     * Look if the list has at least one attribute in common with another one.
     * @param attrs              Other list.
     */
    [[nodiscard]] bool intersects(const RHGAAttrList& attrs) const;

    /**
     * Count the attributes in common with another list.
     * @param attrs              Other list.
     */
    [[nodiscard]] std::size_t nbCommon(const RHGAAttrList& attrs) const;

    /**
     * Compute the intersection of two lists.
     * @param first              First list.
     * @param second             Second list.
     */
    [[nodiscard]] static RHGAAttrList inter(const RHGAAttrList& first, const RHGAAttrList& second);

    /**
     * Compute the attributes of a list that are not in another one.
     * @param attrs              List.
     * @param removed            Attributes to remove.
     */
    [[nodiscard]] static RHGAAttrList diff(const RHGAAttrList& attrs, const RHGAAttrList& removed);

    /**
     * Get a string representation of the list ("1,2,3").
     */
    [[nodiscard]] std::string toString() const;

    /**
     * Compare two lists.
     * @param attrs              Other list.
     */
    [[nodiscard]] bool operator==(const RHGAAttrList& attrs) const { return attrs_ == attrs.attrs_; }

    /**
     * Compare two lists.
     * @param attrs              Other list.
     */
    [[nodiscard]] bool operator!=(const RHGAAttrList& attrs) const { return attrs_ != attrs.attrs_; }

    /**
     * Get an iterator on the first attribute.
     */
    [[nodiscard]] auto begin() const { return attrs_.begin(); }

    /**
     * Get an iterator after the last attribute.
     */
    [[nodiscard]] auto end() const { return attrs_.end(); }

private:

    //-----------------------------------------------------
    std::vector<std::size_t> attrs_;        ///< Attributes (ordered, without duplicates).
};


}  //------- End of namespace evocomb ----------------------------------------------
