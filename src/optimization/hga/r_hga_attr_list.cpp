/*

    EvoComb

    RHGAAttrList.cpp

    List of Attributes of the Hierarchical Genetic Algorithm - Implementation.

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
// include files for ANSI C/C++
#include <algorithm>
#include <iterator>
#include <utility>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "hga/r_hga_attr_list.hpp"
using namespace evocomb;



//------------------------------------------------------------------------------
//
// class RHGAAttrList
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
RHGAAttrList::RHGAAttrList(const std::initializer_list<std::size_t> attrs) :
    RHGAAttrList(std::vector<std::size_t>(attrs))
{
}


//------------------------------------------------------------------------------
RHGAAttrList::RHGAAttrList(std::vector<std::size_t> attrs) :
    attrs_(std::move(attrs))
{
    std::sort(attrs_.begin(), attrs_.end());
    attrs_.erase(std::unique(attrs_.begin(), attrs_.end()), attrs_.end());
}


//------------------------------------------------------------------------------
void RHGAAttrList::insert(const std::size_t attr)
{
    const auto it = std::lower_bound(attrs_.begin(), attrs_.end(), attr);
    if ((it == attrs_.end()) || (*it != attr))
        attrs_.insert(it, attr);
}


//------------------------------------------------------------------------------
bool RHGAAttrList::contains(const std::size_t attr) const
{
    return std::binary_search(attrs_.begin(), attrs_.end(), attr);
}


//------------------------------------------------------------------------------
bool RHGAAttrList::isParentOf(const RHGAAttrList& child) const
{
    return std::includes(child.attrs_.begin(), child.attrs_.end(), attrs_.begin(), attrs_.end());
}


//------------------------------------------------------------------------------
bool RHGAAttrList::intersects(const RHGAAttrList& attrs) const
{
    auto it1 = attrs_.begin();
    auto it2 = attrs.attrs_.begin();
    while ((it1 != attrs_.end()) && (it2 != attrs.attrs_.end()))
    {
        if (*it1 == *it2)
            return true;
        if (*it1 < *it2)
            ++it1;
        else
            ++it2;
    }
    return false;
}


//------------------------------------------------------------------------------
std::size_t RHGAAttrList::nbCommon(const RHGAAttrList& attrs) const
{
    std::size_t nb = 0;
    auto it1 = attrs_.begin();
    auto it2 = attrs.attrs_.begin();
    while ((it1 != attrs_.end()) && (it2 != attrs.attrs_.end()))
    {
        if (*it1 == *it2)
        {
            ++nb;
            ++it1;
            ++it2;
        }
        else if (*it1 < *it2)
            ++it1;
        else
            ++it2;
    }
    return nb;
}


//------------------------------------------------------------------------------
RHGAAttrList RHGAAttrList::inter(const RHGAAttrList& first, const RHGAAttrList& second)
{
    RHGAAttrList res;
    std::set_intersection(first.attrs_.begin(), first.attrs_.end(), second.attrs_.begin(), second.attrs_.end(), std::back_inserter(res.attrs_));
    return res;
}


//------------------------------------------------------------------------------
RHGAAttrList RHGAAttrList::diff(const RHGAAttrList& attrs, const RHGAAttrList& removed)
{
    RHGAAttrList res;
    std::set_difference(attrs.attrs_.begin(), attrs.attrs_.end(), removed.attrs_.begin(), removed.attrs_.end(), std::back_inserter(res.attrs_));
    return res;
}


//------------------------------------------------------------------------------
std::string RHGAAttrList::toString() const
{
    std::string str;
    for (const auto attr : attrs_)
    {
        if (!str.empty())
            str += ',';
        str += std::to_string(attr);
    }
    return str;
}
