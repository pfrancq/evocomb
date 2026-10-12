/*

    EvoComb

    RNNGGAData.hpp

    Data of the Nearest Neighbors Grouping Genetic Algorithm - Header.

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
#include <vector>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "matrix/r_max_matrix.hpp"
#include "matrix/r_max_vector.hpp"


//------------------------------------------------------------------------------
namespace evocomb {
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
 * The RNNGGAData class provides the data of the objects to group used by the Nearest Neighbors Grouping Genetic
 * Algorithm (NNGGA). For each object, it gives three sparse lists of neighbors ordered by decreasing values (see
 * RMaxVector):
 * - The similarities with the nearest neighbors.
 * - The agreement ratios (for example, the ratio of common positive
 *   assessments of two users).
 * - The disagreement ratios.
 *
 * It also indicates if an object is "social", i.e. it should not remain alone in a group.
 *
 * An application implements this interface, or uses RNNGGAMatrixData.
 * @short NNGGA Data.
 */
class RNNGGAData
{
public:

    /**
     * Destruct the data.
     */
    virtual ~RNNGGAData() = default;

    /**
     * Get the number of objects.
     */
    [[nodiscard]] virtual std::size_t nbObjs() const = 0;

    /**
     * Get the similarities of an object with its nearest neighbors.
     * @param id                 Identifier of the object.
     */
    [[nodiscard]] virtual const RMaxVector& sims(std::size_t id) const = 0;

    /**
     * Get the agreement ratios of an object with other ones.
     * @param id                 Identifier of the object.
     */
    [[nodiscard]] virtual const RMaxVector& agreements(std::size_t id) const = 0;

    /**
     * Get the disagreement ratios of an object with other ones.
     * @param id                 Identifier of the object.
     */
    [[nodiscard]] virtual const RMaxVector& disagreements(std::size_t id) const = 0;

    /**
     * Look if an object is social (it should not remain alone in a group). By default, no object is social.
     * @param id                 Identifier of the object.
     */
    [[nodiscard]] virtual bool isSocial([[maybe_unused]] std::size_t id) const { return false; }
};


//------------------------------------------------------------------------------
/**
 * The RNNGGAMatrixData class provides NNGGA data stored in three sparse matrices (see RMaxMatrix), which keep a maximal
 * number of neighbors for each object:
 * @code
 * evocomb::RNNGGAMatrixData data(nbObjs, 20);      // 20 nearest neighbors
 * data.simMatrix().add(0, 1, 0.8);             // sim(0,1)=0.8
 * data.simMatrix().add(1, 0, 0.8);
 * data.social(3, true);
 * @endcode
 * @short NNGGA Matrix Data.
 */
class RNNGGAMatrixData : public RNNGGAData
{
public:

    /**
     * Construct the data.
     * @param nbObjs             Number of objects.
     * @param nbNeighbors        Maximal number of neighbors kept for each object (0 means no limit).
     */
    RNNGGAMatrixData(std::size_t nbObjs, std::size_t nbNeighbors);

    /**
     * Get the number of objects.
     */
    [[nodiscard]] std::size_t nbObjs() const override { return social_.size(); }

    /**
     * Get the similarities of an object.
     * @param id                 Identifier of the object.
     */
    [[nodiscard]] const RMaxVector& sims(const std::size_t id) const override { return sims_[id]; }

    /**
     * Get the agreement ratios of an object.
     * @param id                 Identifier of the object.
     */
    [[nodiscard]] const RMaxVector& agreements(const std::size_t id) const override { return agreements_[id]; }

    /**
     * Get the disagreement ratios of an object.
     * @param id                 Identifier of the object.
     */
    [[nodiscard]] const RMaxVector& disagreements(const std::size_t id) const override { return disagreements_[id]; }

    /**
     * Look if an object is social.
     * @param id                 Identifier of the object.
     */
    [[nodiscard]] bool isSocial(const std::size_t id) const override { return social_[id]; }

    /**
     * Set if an object is social.
     * @param id                 Identifier of the object.
     * @param social             Social?
     */
    void social(const std::size_t id, const bool social) { social_[id] = social; }

    /**
     * Get the matrix of similarities (to fill it).
     */
    [[nodiscard]] RMaxMatrix& simMatrix() { return sims_; }

    /**
     * Get the matrix of agreement ratios (to fill it).
     */
    [[nodiscard]] RMaxMatrix& agreementMatrix() { return agreements_; }

    /**
     * Get the matrix of disagreement ratios (to fill it).
     */
    [[nodiscard]] RMaxMatrix& disagreementMatrix() { return disagreements_; }

private:

    //-----------------------------------------------------
    RMaxMatrix sims_;                       ///< Similarities.
    RMaxMatrix agreements_;                 ///< Agreement ratios.
    RMaxMatrix disagreements_;              ///< Disagreement ratios.
    std::vector<char> social_;              ///< Social objects.
};


}  //------- End of namespace evocomb ----------------------------------------------
