/*

    EvoComb

    RGA2DChromo.hpp

    Chromosome of the 2D Placement Genetic Algorithm - Header.

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
#include <memory>
#include <string>
#include <vector>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "app/r_logger.hpp"
#include "ga/r_ga.hpp"
#include "ga/r_ga_chromo.hpp"
#include "ga/r_ga_fitness.hpp"
#include "ga2d/r_ga2d_context.hpp"
#include "ga2d/r_ga2d_heuristic.hpp"
#include "ga2d/r_ga2d_layout.hpp"


//------------------------------------------------------------------------------
namespace evocomb {
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// forward declaration
template<class C> class RGA2DPopulation;


//------------------------------------------------------------------------------
/**
 * The RGA2DThreadData class provides the thread-dependent data of the 2D placement genetic algorithm: the grid, the
 * heuristic (created by createHeuristic from the name given by the context), the PROMETHEE kernel of the crossover and
 * some temporary structures.
 *
 * A problem can derive from this class to add its own heuristics. The constructor must keep the same parameters (see
 * RGA2DPopulation). Once constructed, the data are initialized by init.
 * @short 2D Placement Thread-dependent Data.
 */
class RGA2DThreadData
{
public:

    /**
     * Construct the data.
     * @param logger             Logger.
     * @param context            Context.
     * @exception RException     The selection criteria are invalid.
     */
    RGA2DThreadData(RLogger& logger, const RGA2DContext& context);

    /**
     * Destruct the data.
     */
    virtual ~RGA2DThreadData() = default;

    /**
     * Initialize the data: the heuristic given by the context is created.
     * @param logger             Logger.
     * @exception RGAException   The heuristic is unknown.
     */
    virtual void init(RLogger& logger);

    /**
     * Get the context.
     */
    [[nodiscard]] const RGA2DContext& context() const { return context_; }

    /**
     * Get the grid.
     */
    [[nodiscard]] RGA2DGrid& grid() { return grid_; }

    /**
     * Get the heuristic.
     */
    [[nodiscard]] RGA2DHeuristic& heuristic() { return *heuristic_; }

    /**
     * Get the PROMETHEE kernel choosing the objects kept by the crossover.
     */
    [[nodiscard]] RGAPromFitness& selection() { return selection_; }

    /**
     * Get the flags of the objects selected by the crossover.
     */
    [[nodiscard]] std::vector<char>& selected() { return selected_; }

    /**
     * Get the blocks built by the crossover.
     */
    [[nodiscard]] std::vector<RGA2DBlock>& blocks() { return blocks_; }

    //-----------------------------------------------------
    RGA2DThreadData(const RGA2DThreadData&) = delete;
    RGA2DThreadData& operator=(const RGA2DThreadData&) = delete;

protected:

    /**
     * Create a heuristic from its name ("bottom_left", "edge" or "center").
     * @param logger             Logger.
     * @param name               Name.
     * @exception RGAException   The heuristic is unknown.
     */
    [[nodiscard]] virtual std::unique_ptr<RGA2DHeuristic> createHeuristic(RLogger& logger, const std::string& name);

private:

    //-----------------------------------------------------
    const RGA2DContext& context_;           ///< Context.
    RGA2DGrid grid_;                        ///< Grid.
    std::unique_ptr<RGA2DHeuristic> heuristic_; ///< Heuristic.
    RGAPromFitness selection_;              ///< Kernel of the crossover.
    std::vector<char> selected_;            ///< Objects selected.
    std::vector<RGA2DBlock> blocks_;        ///< Blocks.
};


//------------------------------------------------------------------------------
/**
 * The RGA2DChromo class provides a chromosome of the 2D placement genetic algorithm: a complete placement of the
 * objects (see RGA2DLayout).
 * - The random construction places all the objects with the heuristic.
 * - The crossover keeps a group of objects from each parent (with their
 *   relative positions): the most connected object (chosen with PROMETHEE
 *   II), a second one such that both fit in a quarter of the placement of
 *   the parent, and all the objects inside their bounding rectangle. These
 *   two groups are placed as blocks, with the other objects, by the
 *   heuristic.
 * - The mutation builds a new placement with the heuristic.
 *
 * The chromosome computes two criteria (evaluate):
 * - Criterion 0: the area of the bounding rectangle of the placement divided
 *   by the area of the limits (to minimize).
 * - Criterion 1: the length of the nets (weighted if "weighted_distances" is
 *   true), to minimize.
 *
 * The default fitness strategy uses the first criterion only (with "maximize": false), both can be combined with
 * PROMETHEE.
 * @short 2D Placement Chromosome.
 */
class RGA2DChromo : public RGAChromo<RGA2DThreadData>, public RGA2DLayout
{
public:

    /**
     * Class of the context.
     */
    using Context = RGA2DContext;

    /**
     * Get the context (available once the population is initialized).
     */
    [[nodiscard]] const RGA2DContext& context() const { return *context_; }

    /**
     * Construct a placement with the heuristic.
     * @param logger             Logger.
     * @param data               Thread-dependent data.
     * @param random             Random generator.
     */
    void randomConstruct(RLogger& logger, RGA2DThreadData& data, RGARandom& random) override;

    /**
     * Do a crossover (see the class description).
     * @param logger             Logger.
     * @param data               Thread-dependent data.
     * @param random             Random generator.
     * @param parent1            First parent.
     * @param parent2            Second parent.
     */
    void crossover(RLogger& logger, RGA2DThreadData& data, RGARandom& random, const RGA2DChromo& parent1, const RGA2DChromo& parent2);

    /**
     * Do a mutation: a new placement is built by the heuristic.
     * @param logger             Logger.
     * @param data               Thread-dependent data.
     * @param random             Random generator.
     */
    void mutation(RLogger& logger, RGA2DThreadData& data, RGARandom& random) override;

    /**
     * Compute the criteria (see the class description).
     * @param logger             Logger.
     * @param data               Thread-dependent data.
     */
    void evaluate(RLogger& logger, RGA2DThreadData& data) override;

    /**
     * Verify the placement: all the objects are placed, inside the limits and without overlaps.
     * @param logger             Logger.
     * @param data               Thread-dependent data.
     * @exception RGAException   The placement is invalid.
     */
    void verify(RLogger& logger, RGA2DThreadData& data) const override;

    /**
     * Look if two chromosomes represent the same placement.
     * @param chromo             Chromosome.
     */
    [[nodiscard]] bool isSame(const RGA2DChromo& chromo) const { return sameLayout(chromo); }

    /**
     * Get the area ratio (criterion 0, after an evaluation).
     */
    [[nodiscard]] double areaRatio() const { return (criteria().size() > 0) ? criteria()[0] : 0.0; }

    /**
     * Get the length of the nets (criterion 1, after an evaluation).
     */
    [[nodiscard]] double length() const { return (criteria().size() > 1) ? criteria()[1] : 0.0; }

private:

    /**
     * Select the objects kept from a parent by the crossover (see the class description).
     * @param logger             Logger.
     * @param data               Thread-dependent data.
     * @param random             Random generator.
     * @param parent             Parent.
     * @return the objects selected (they are also marked in the thread-dependent data).
     */
    std::vector<std::size_t> selectObjs(RLogger& logger, RGA2DThreadData& data, RGARandom& random, const RGA2DLayout& parent) const;

    /**
     * Set the context (called by the population).
     * @param context            Context.
     */
    void setContext(const RGA2DContext& context);

    //-----------------------------------------------------
    const RGA2DContext* context_ = nullptr; ///< Context.

    //-----------------------------------------------------
    template<class> friend class RGA2DPopulation;
};


}  //------- End of namespace evocomb ----------------------------------------------
