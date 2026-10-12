/*

    EvoComb - VLSI example

    RVLSIProblem.hpp

    VLSI Placement Problem - Header.

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
#include <memory>
#include <string>
#include <vector>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "ga2d/r_ga2d_chromo.hpp"
#include "ga2d/r_ga2d_problem.hpp"
#include "tools/r_json.hpp"


//------------------------------------------------------------------------------
namespace evocomb::vlsi {
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
 * The RVLSIProblem class provides a VLSI placement problem: instances of masters (cells) must be placed on a circuit,
 * and their terminals are connected by nets. It is read from a JSON document (the coordinates are those of the corners
 * of the unit squares, see RRect):
 * @code
 * {
 *   "size": [80, 80],                                  // Placement area.
 *   "terminals": [                                     // Terminals of the circuit (optional).
 *     {"id": "E", "pins": [{"id": "p1", "layer": "Met2", "rect": [-2, 80, 0, 82]}]}
 *   ],
 *   "masters": [
 *     {"id": "Master1",
 *      "polygon": [[0, 0], [10, 0], [10, 10], [0, 10]],  // Rectilinear shape.
 *      "terminals": [{"id": "A", "pins": [{"id": "Pin1", "layer": "Met1", "rect": [3, 3, 5, 5]}]}]}
 *   ],
 *   "instances": [{"id": "Inst0", "master": "Master1"}],
 *   "nets": [
 *     {"id": "net1", "weight": 1,                      // Weight (optional, 1 by default).
 *      "connect": [{"instance": "Inst0", "terminal": "A"},
 *                  {"terminal": "E"}]}                 // A terminal of the circuit.
 *   ]
 * }
 * @endcode
 * Each instance becomes an object of a 2D placement problem (see RGA2DProblem). The "id" of the pins and their "layer"
 * are optional.
 * @short VLSI Problem.
 */
class RVLSIProblem
{
public:

    /**
     * Construct an empty problem.
     */
    RVLSIProblem() = default;

    /**
     * Read the problem from a JSON document (see the class description). Other elements (for example a "config" object)
     * are ignored.
     * @param json               JSON document.
     * @exception RException     The document is invalid.
     */
    void read(const RJSON& json);

    /**
     * Get the placement problem (initialized).
     */
    [[nodiscard]] const RGA2DProblem& problem() const { return *problem_; }

    /**
     * Get the master of an instance.
     * @param id                 Identifier of the instance (object).
     */
    [[nodiscard]] const std::string& master(const std::size_t id) const { return masters_[id]; }

    /**
     * Export a placement in JSON: an array with, for each instance, its identifier, its master, its orientation, its
     * position (the bottom-left corner of its bounding rectangle) and its size:
     * @code
     * [{"id": "Inst0", "master": "Master1", "orientation": "normal", "position": [0, 0], "size": [10, 10]}]
     * @endcode
     * @param layout             Placement.
     */
    [[nodiscard]] std::unique_ptr<RJSON> toJSON(const RGA2DLayout& layout) const;

    //-----------------------------------------------------
    RVLSIProblem(const RVLSIProblem&) = delete;
    RVLSIProblem& operator=(const RVLSIProblem&) = delete;

private:

    //-----------------------------------------------------
    std::unique_ptr<RGA2DProblem> problem_; ///< Placement problem.
    std::vector<std::string> masters_;      ///< Master of each instance.
};


}  //------- End of namespace evocomb::vlsi ----------------------------------------
