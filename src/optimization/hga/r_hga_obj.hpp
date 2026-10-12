/*

    EvoComb

    RHGAObj.hpp

    Object of the Hierarchical Genetic Algorithm - Header.

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
#include <string>
#include <utility>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "hga/r_hga_attr_list.hpp"


//------------------------------------------------------------------------------
namespace evocomb {
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
 * The RHGAObj class provides an object to place in the tree built by the Hierarchical Genetic Algorithm: an identifier,
 * a name and a list of attributes (see RHGAAttrList).
 *
 * The HGA can use any class of objects providing:
 * @code
 * std::size_t id() const;                   // In [0, number of objects[.
 * const evocomb::RHGAAttrList& attrs() const;   // Not empty.
 * @endcode
 * @short HGA Object.
 */
class RHGAObj
{
public:

    /**
     * Construct the object.
     * @param id                 Identifier.
     * @param name               Name.
     * @param attrs              Attributes.
     */
    RHGAObj(const std::size_t id, std::string name, RHGAAttrList attrs = {}) :
        id_(id),
        name_(std::move(name)),
        attrs_(std::move(attrs))
    {
    }

    /**
     * Get the identifier of the object.
     */
    [[nodiscard]] std::size_t id() const { return id_; }

    /**
     * Get the name of the object.
     */
    [[nodiscard]] const std::string& name() const { return name_; }

    /**
     * Get the attributes of the object.
     */
    [[nodiscard]] const RHGAAttrList& attrs() const { return attrs_; }

    /**
     * Add an attribute to the object.
     * @param attr               Attribute.
     */
    void addAttr(const std::size_t attr) { attrs_.insert(attr); }

private:

    //-----------------------------------------------------
    std::size_t id_;                        ///< Identifier.
    std::string name_;                      ///< Name.
    RHGAAttrList attrs_;                    ///< Attributes.
};


}  //------- End of namespace evocomb ----------------------------------------------
