/*

    EvoComb - Bin Packing example

    RBPProblem.hpp

    Bin Packing Problem - Header.

    Copyright 1998-2026 by Pascal Francq (pascal@francq.info).

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/



//------------------------------------------------------------------------------
#pragma once


//------------------------------------------------------------------------------
// include files for ANSI C/C++
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "tools/r_json.hpp"


//------------------------------------------------------------------------------
namespace evocomb::bp {
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
 * The RObjBP class provides an object (item) of the bin packing problem.
 * @short Bin Packing Object.
 */
class RObjBP
{
public:

    /**
     * Construct the object.
     * @param id                 Identifier.
     * @param size               Size.
     */
    RObjBP(const std::size_t id, const std::size_t size) : id_(id), size_(size) {}

    /**
     * Get the identifier.
     */
    [[nodiscard]] std::size_t id() const { return id_; }

    /**
     * Get the size.
     */
    [[nodiscard]] std::size_t size() const { return size_; }

private:

    //-----------------------------------------------------
    std::size_t id_;                        ///< Identifier.
    std::size_t size_;                      ///< Size.
};


//------------------------------------------------------------------------------
/**
 * The RBPProblem class provides an instance of the bin packing problem: a set of objects to put in bins with a given
 * capacity, by using as few bins as possible.
 * @short Bin Packing Problem.
 */
class RBPProblem
{
public:

    /**
     * Construct an empty problem.
     */
    RBPProblem() = default;

    /**
     * Load a problem from a text file with the format (integers separated by spaces or new lines):
     * @code
     * number_of_objects
     * size_1 size_2 ... size_n
     * bin_capacity
     * minimal_number_of_bins       (optional, 0 if unknown)
     * @endcode
     * @param path               Path of the file.
     * @exception RException     The file cannot be read or is invalid.
     */
    void load(const std::filesystem::path& path);

    /**
     * Read a problem from a JSON document. The objects are given either by their sizes:
     * @code
     * {
     *   "capacity": 150,               // Capacity of the bins (mandatory).
     *   "objects": [42, 69, 67, ...],  // Sizes of the objects (mandatory).
     *   "min_bins": 3                  // Minimal number of bins (optional, 0 if unknown).
     * }
     * @endcode
     * or generated (see generate):
     * @code
     * {"generate": {"nb_bins": 30, "capacity": 150, "seed": 42}}
     * @endcode
     * Other elements (for example a "config" object) are ignored.
     * @param json               JSON document.
     * @exception RException     The document is invalid.
     */
    void read(const RJSON& json);

    /**
     * Generate a problem with a known optimal solution: each bin is filled exactly by splitting its capacity into 2 to
     * 4 objects.
     * @param nbBins             Number of bins of the optimal solution.
     * @param capacity           Capacity of the bins.
     * @param seed               Seed of the random generator.
     */
    void generate(std::size_t nbBins, std::size_t capacity, std::uint32_t seed);

    /**
     * Clear the problem.
     */
    void clear();

    /**
     * Get the capacity of the bins.
     */
    [[nodiscard]] std::size_t capacity() const { return capacity_; }

    /**
     * Get the theoretical minimal number of bins (0 if unknown).
     */
    [[nodiscard]] std::size_t minBins() const { return minBins_; }

    /**
     * Get the lower bound of the number of bins: the total size divided by the capacity (rounded up).
     */
    [[nodiscard]] std::size_t lowerBound() const;

    /**
     * Get the objects.
     */
    [[nodiscard]] const std::vector<RObjBP>& objs() const { return objs_; }

    /**
     * Get pointers to the objects (as needed by the GGA context).
     */
    [[nodiscard]] std::vector<const RObjBP*> objPtrs() const;

private:

    //-----------------------------------------------------
    std::vector<RObjBP> objs_;              ///< Objects.
    std::size_t capacity_ = 0;              ///< Capacity of the bins.
    std::size_t minBins_ = 0;               ///< Minimal number of bins (0 if unknown).
};


}  //------- End of namespace evocomb::bp ------------------------------------------
