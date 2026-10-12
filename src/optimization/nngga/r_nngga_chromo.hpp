/*

    EvoComb

    RNNGGAChromo.hpp

    Chromosome of the Nearest Neighbors Grouping Genetic Algorithm - Header.

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
#include <type_traits>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "app/r_logger.hpp"
#include "gga/r_gga_chromo.hpp"
#include "gga/r_gga_population.hpp"
#include "nngga/r_nngga_context.hpp"
#include "nngga/r_nngga_group.hpp"
#include "nngga/r_nngga_heuristic.hpp"


//------------------------------------------------------------------------------
namespace evocomb {
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
 * The RNNGGAThreadData class provides the thread-dependent data of the Nearest Neighbors Grouping Genetic Algorithm. It
 * adds the heuristic "nn_first_fit" (see RNNGGAHeuristic).
 * @tparam G                     Class of the groups.
 * @tparam O                     Class of the objects.
 * @short NNGGA Thread-dependent Data.
 */
template<class G, class O>
    class RNNGGAThreadData : public RGGAThreadData<G, O, RNNGGAContext<O>>
{
public:

    /**
     * Class of the heuristics.
     */
    using Heuristic = RGGAHeuristic<G, O, RNNGGAContext<O>>;

    /**
     * Construct the data.
     * @param logger             Logger.
     * @param context            Context.
     */
    RNNGGAThreadData(RLogger& logger, const RNNGGAContext<O>& context) :
        RGGAThreadData<G, O, RNNGGAContext<O>>(logger, context)
    {
    }

protected:

    /**
     * Create a heuristic from its name ("nn_first_fit" or "first_fit").
     * @param logger             Logger.
     * @param name               Name.
     * @exception RGAException   The heuristic is unknown.
     */
    [[nodiscard]] std::unique_ptr<Heuristic> createHeuristic(RLogger& logger, const std::string& name) override
    {
        if (name == "nn_first_fit")
            return std::make_unique<RNNGGAHeuristic<G, O>>(this->context());
        return RGGAThreadData<G, O, RNNGGAContext<O>>::createHeuristic(logger, name);
    }
};


//------------------------------------------------------------------------------
/**
 * The RNNGGAChromo class provides a chromosome of the Nearest Neighbors Grouping Genetic Algorithm (NNGGA), a GGA
 * clustering objects from the lists of their nearest neighbors (see RNNGGAData).
 *
 * The chromosome computes three criteria (evaluate):
 * - Criterion 0, similarity "J": the average similarity between the objects
 *   of the same group divided by the average similarity between the objects
 *   of different groups (only the similarities of the nearest neighbors are
 *   considered).
 * - Criterion 1, agreement: the same ratio for the agreement ratios.
 * - Criterion 2, disagreement: the same ratio for the disagreement ratios.
 *
 * The first two must be maximized and the last one minimized, typically with the PROMETHEE fitness:
 * @code
 * "fitness": {"type": "promethee", "criteria": [
 *     {"name": "similarity", "type": "maximize"},
 *     {"name": "agreement", "type": "maximize"},
 *     {"name": "disagreement", "type": "minimize"}]}
 * @endcode
 * @tparam O                     Class of the objects.
 * @tparam G                     Class of the groups (it must inherit from RNNGGAGroup<O>).
 * @tparam T                     Class of the thread-dependent data (it must inherit from RNNGGAThreadData<G, O>).
 * @short NNGGA Chromosome.
 */
template<class O, class G = RNNGGAGroup<O>, class T = RNNGGAThreadData<G, O>>
    class RNNGGAChromo : public RGGAChromo<G, O, T, RNNGGAContext<O>>
{
    static_assert(std::is_base_of_v<RNNGGAGroup<O>, G>, "G must inherit from RNNGGAGroup<O>");

public:

    /**
     * Compute the three criteria (see the class description).
     * @param logger             Logger.
     * @param data               Thread-dependent data.
     */
    void evaluate([[maybe_unused]] RLogger& logger, [[maybe_unused]] T& data) override
    {
        const auto& nnData = this->context().data();
        Ratio sim;
        Ratio agree;
        Ratio disagree;
        for (std::size_t i = 0; i < this->nbObjs(); ++i)
        {
            const auto grp = this->groupId(i);
            sim.add(*this, nnData.sims(i), grp);
            agree.add(*this, nnData.agreements(i), grp);
            disagree.add(*this, nnData.disagreements(i), grp);
        }
        this->criterion(0, sim.compute());
        this->criterion(1, agree.compute());
        this->criterion(2, disagree.compute());
    }

    /**
     * Look if two objects are in the same group.
     * @param id1                Identifier of the first object.
     * @param id2                Identifier of the second object.
     */
    [[nodiscard]] bool sameGroup(const std::size_t id1, const std::size_t id2) const { return this->groupId(id1) == this->groupId(id2); }

    /**
     * Get the similarity criterion (after an evaluation).
     */
    [[nodiscard]] double simJ() const { return criterionValue(0); }

    /**
     * Get the agreement criterion (after an evaluation).
     */
    [[nodiscard]] double agreement() const { return criterionValue(1); }

    /**
     * Get the disagreement criterion (after an evaluation).
     */
    [[nodiscard]] double disagreement() const { return criterionValue(2); }

protected:

    /**
     * Create a group with the context.
     * @param id                 Identifier of the group.
     */
    [[nodiscard]] G newGroup(const std::size_t id) const override { return G(id, this->context()); }

private:

    /**
     * Ratio between the average value inside the groups and the average value between the groups.
     */
    struct Ratio
    {
        double intra = 0.0;                 ///< Sum of the values inside the groups.
        double inter = 0.0;                 ///< Sum of the values between the groups.
        std::size_t nbIntra = 0;            ///< Number of values inside the groups.
        std::size_t nbInter = 0;            ///< Number of values between the groups.

        /**
         * Add the values of the neighbors of an object.
         * @param chromo         Chromosome.
         * @param neighbors      Neighbors of the object.
         * @param grp            Group of the object.
         */
        void add(const RNNGGAChromo& chromo, const RMaxVector& neighbors, const std::size_t grp)
        {
            for (const auto& [id, value] : neighbors)
            {
                if (chromo.groupId(id) == grp)
                {
                    intra += value;
                    ++nbIntra;
                }
                else
                {
                    inter += value;
                    ++nbInter;
                }
            }
        }

        /**
         * Compute the ratio: average intra / average inter (the average intra if there is no value between the groups,
         * 0 if there is no value inside the groups).
         */
        [[nodiscard]] double compute() const
        {
            if (nbIntra == 0)
                return 0.0;
            const auto avgIntra = intra / static_cast<double>(nbIntra);
            if (nbInter == 0)
                return avgIntra;
            return avgIntra / (inter / static_cast<double>(nbInter));
        }
    };

    /**
     * Get the value of a criterion (0 if not computed).
     * @param idx                Index of the criterion.
     */
    [[nodiscard]] double criterionValue(const std::size_t idx) const
    {
        return idx < this->criteria().size() ? this->criteria()[idx] : 0.0;
    }
};


//------------------------------------------------------------------------------
/**
 * Population of the Nearest Neighbors Grouping Genetic Algorithm:
 * @code
 * evocomb::RNNGGAMatrixData data(nbObjs, 20);
 * // ... fill the data ...
 * evocomb::RNNGGAPopulation<MyObj> population(32, evocomb::RNNGGAContext<MyObj>(objs, data), config);
 * evocomb::RGAInst inst(logger, config);
 * inst.init(&population);
 * inst.run(100);
 * @endcode
 * @tparam O                     Class of the objects.
 */
template<class O>
    using RNNGGAPopulation = RGGAPopulation<RNNGGAChromo<O>>;


}  //------- End of namespace evocomb ----------------------------------------------
