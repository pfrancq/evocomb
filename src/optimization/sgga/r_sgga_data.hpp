/*

    EvoComb

    RSGGAData.hpp

    Data of the Similarity-based Grouping Genetic Algorithm - Header.

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
#include "matrix/r_symmetric_matrix.hpp"
#include "tools/r_std.hpp"


//------------------------------------------------------------------------------
namespace evocomb {
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
 * The RSGGAData class provides the data of the objects to group used by the Similarity-based Grouping Genetic Algorithm
 * (SGGA). Contrary to the NNGGA (see RNNGGAData), the measures are available for every pair of objects:
 * - The similarity (typically in [-1, 1]).
 * - The agreement ratio (for example, the ratio of common positive
 *   assessments of two users).
 * - The disagreement ratio.
 *
 * It also indicates if an object is "social" (it should not remain alone in a group) and the "parent" of an object: two
 * objects with the same parent cannot be grouped together (for example two profiles of the same user).
 *
 * An application implements this interface, or uses RSGGAMatrixData.
 * @short SGGA Data.
 */
class RSGGAData
{
public:

    /**
     * Destruct the data.
     */
    virtual ~RSGGAData() = default;

    /**
     * Get the number of objects.
     */
    [[nodiscard]] virtual std::size_t nbObjs() const = 0;

    /**
     * Get the similarity between two objects.
     * @param id1                Identifier of the first object.
     * @param id2                Identifier of the second object.
     */
    [[nodiscard]] virtual double sim(std::size_t id1, std::size_t id2) const = 0;

    /**
     * Get the agreement ratio between two objects.
     * @param id1                Identifier of the first object.
     * @param id2                Identifier of the second object.
     */
    [[nodiscard]] virtual double agreement(std::size_t id1, std::size_t id2) const = 0;

    /**
     * Get the disagreement ratio between two objects.
     * @param id1                Identifier of the first object.
     * @param id2                Identifier of the second object.
     */
    [[nodiscard]] virtual double disagreement(std::size_t id1, std::size_t id2) const = 0;

    /**
     * Look if an object is social (it should not remain alone in a group). By default, no object is social.
     * @param id                 Identifier of the object.
     */
    [[nodiscard]] virtual bool isSocial([[maybe_unused]] std::size_t id) const { return false; }

    /**
     * Get the parent of an object: two objects with the same parent cannot be grouped together. By default, the objects
     * have no parent.
     * @param id                 Identifier of the object.
     * @return cNoRef if the object has no parent.
     */
    [[nodiscard]] virtual std::size_t parent([[maybe_unused]] std::size_t id) const { return cNoRef; }
};


//------------------------------------------------------------------------------
/**
 * The RSGGAMatrixData class provides SGGA data stored in three symmetric matrices (the measures not set are null):
 * @code
 * evocomb::RSGGAMatrixData data(nbObjs);
 * data.sim(0, 1, 0.8);                 // sim(0,1)=sim(1,0)=0.8
 * data.disagreement(0, 2, 0.9);
 * data.social(3, true);
 * data.parent(4, 12);
 * @endcode
 * @short SGGA Matrix Data.
 */
class RSGGAMatrixData : public RSGGAData
{
public:

    /**
     * Construct the data.
     * @param nbObjs             Number of objects.
     */
    explicit RSGGAMatrixData(std::size_t nbObjs);

    /**
     * Get the number of objects.
     */
    [[nodiscard]] std::size_t nbObjs() const override { return social_.size(); }

    /**
     * Get the similarity between two objects.
     * @param id1                Identifier of the first object.
     * @param id2                Identifier of the second object.
     */
    [[nodiscard]] double sim(const std::size_t id1, const std::size_t id2) const override { return sims_(id1, id2); }

    /**
     * Set the similarity between two objects.
     * @param id1                Identifier of the first object.
     * @param id2                Identifier of the second object.
     * @param value              Value.
     */
    void sim(const std::size_t id1, const std::size_t id2, const double value) { sims_(id1, id2) = value; }

    /**
     * Get the agreement ratio between two objects.
     * @param id1                Identifier of the first object.
     * @param id2                Identifier of the second object.
     */
    [[nodiscard]] double agreement(const std::size_t id1, const std::size_t id2) const override { return agreements_(id1, id2); }

    /**
     * Set the agreement ratio between two objects.
     * @param id1                Identifier of the first object.
     * @param id2                Identifier of the second object.
     * @param value              Value.
     */
    void agreement(const std::size_t id1, const std::size_t id2, const double value) { agreements_(id1, id2) = value; }

    /**
     * Get the disagreement ratio between two objects.
     * @param id1                Identifier of the first object.
     * @param id2                Identifier of the second object.
     */
    [[nodiscard]] double disagreement(const std::size_t id1, const std::size_t id2) const override { return disagreements_(id1, id2); }

    /**
     * Set the disagreement ratio between two objects.
     * @param id1                Identifier of the first object.
     * @param id2                Identifier of the second object.
     * @param value              Value.
     */
    void disagreement(const std::size_t id1, const std::size_t id2, const double value) { disagreements_(id1, id2) = value; }

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
     * Get the parent of an object.
     * @param id                 Identifier of the object.
     */
    [[nodiscard]] std::size_t parent(const std::size_t id) const override { return parents_[id]; }

    /**
     * Set the parent of an object.
     * @param id                 Identifier of the object.
     * @param parent             Identifier of the parent (cNoRef if none).
     */
    void parent(const std::size_t id, const std::size_t parent) { parents_[id] = parent; }

private:

    //-----------------------------------------------------
    RSymmetricMatrix sims_;                 ///< Similarities.
    RSymmetricMatrix agreements_;           ///< Agreement ratios.
    RSymmetricMatrix disagreements_;        ///< Disagreement ratios.
    std::vector<char> social_;              ///< Social objects.
    std::vector<std::size_t> parents_;      ///< Parents of the objects.
};


}  //------- End of namespace evocomb ----------------------------------------------
