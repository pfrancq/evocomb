/*

    EvoComb

    RNNGGAData.cpp

    Data of the Nearest Neighbors Grouping Genetic Algorithm - Implementation.

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
// include files for EvoComb
#include "nngga/r_nngga_data.hpp"
using namespace evocomb;



//------------------------------------------------------------------------------
//
// class RNNGGAMatrixData
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
RNNGGAMatrixData::RNNGGAMatrixData(const std::size_t nbObjs, const std::size_t nbNeighbors) :
    sims_(nbObjs, nbNeighbors),
    agreements_(nbObjs, nbNeighbors),
    disagreements_(nbObjs, nbNeighbors),
    social_(nbObjs, 0)
{
}
