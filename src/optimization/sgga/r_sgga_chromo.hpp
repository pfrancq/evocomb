/*

    EvoComb

    RSGGAChromo.hpp

    Chromosome of the Similarity-based Grouping Genetic Algorithm - Header.

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
#include <vector>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "app/r_logger.hpp"
#include "ga/r_ga_fitness.hpp"
#include "gga/r_gga_chromo.hpp"
#include "gga/r_gga_population.hpp"
#include "sgga/r_sgga_context.hpp"
#include "sgga/r_sgga_group.hpp"
#include "sgga/r_sgga_heuristic.hpp"
#include "sgga/r_sgga_operators.hpp"
#include "sgga/r_sgga_solution.hpp"


//------------------------------------------------------------------------------
namespace evocomb {
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
 * The RSGGAThreadData class provides the thread-dependent data of the Similarity-based Grouping Genetic Algorithm:
 * - The heuristic "sg_first_fit" (see RSGGAHeuristic) is added.
 * - The temporary structures of the operators (see RSGGAWork).
 * - The temporary solutions and the PROMETHEE kernel used by the
 *   optimization of the chromosomes (see RSGGAChromo::optimize).
 * @tparam G                     Class of the groups.
 * @tparam O                     Class of the objects.
 * @short SGGA Thread-dependent Data.
 */
template<class G, class O>
    class RSGGAThreadData : public RGGAThreadData<G, O, RSGGAContext<O>>
{
public:

    /**
     * Class of the heuristics.
     */
    using Heuristic = RGGAHeuristic<G, O, RSGGAContext<O>>;

    /**
     * Class of the temporary solutions.
     */
    using Solution = RSGGASolution<G, O>;

    /**
     * Construct the data.
     * @param logger             Logger.
     * @param context            Context.
     */
    RSGGAThreadData(RLogger& logger, const RSGGAContext<O>& context) :
        RGGAThreadData<G, O, RSGGAContext<O>>(logger, context)
    {
    }

    /**
     * Initialize the data: the heuristic, the temporary solutions (2*nb_div_chromos+1) and the PROMETHEE kernel.
     * @param logger             Logger.
     * @exception RGAException   The heuristic is unknown.
     * @exception RException     The PROMETHEE criteria are invalid.
     */
    void init(RLogger& logger) override
    {
        RGGAThreadData<G, O, RSGGAContext<O>>::init(logger);
        const auto& context = this->context();
        tests_.resize(context.nbDivChromos() * 2 + 1);
        for (auto& test : tests_)
            test.init(context);
        prom_ = std::make_unique<RGAPromFitness>(logger, context.criteria());
    }

    /**
     * Get the temporary structures of the operators.
     */
    [[nodiscard]] RSGGAWork<O>& work() { return work_; }

    /**
     * Get the temporary solutions.
     */
    [[nodiscard]] std::vector<Solution>& tests() { return tests_; }

    /**
     * Get the PROMETHEE kernel (as a fitness strategy).
     */
    [[nodiscard]] RGAPromFitness& prom() { return *prom_; }

    /**
     * Get a temporary vector of criteria values.
     */
    [[nodiscard]] std::vector<double>& tmpCriteria() { return tmpCriteria_; }

    /**
     * Get a temporary vector of pointers to criteria values (the solutions compared with PROMETHEE).
     */
    [[nodiscard]] std::vector<const std::vector<double>*>& tmpSols() { return tmpSols_; }

    /**
     * Get a temporary vector of Fi (the result of the PROMETHEE kernel).
     */
    [[nodiscard]] std::vector<double>& tmpFi() { return tmpFi_; }

protected:

    /**
     * Create a heuristic from its name ("sg_first_fit" or "first_fit").
     * @param logger             Logger.
     * @param name               Name.
     * @exception RGAException   The heuristic is unknown.
     */
    [[nodiscard]] std::unique_ptr<Heuristic> createHeuristic(RLogger& logger, const std::string& name) override
    {
        if (name == "sg_first_fit")
            return std::make_unique<RSGGAHeuristic<G, O>>(this->context());
        return RGGAThreadData<G, O, RSGGAContext<O>>::createHeuristic(logger, name);
    }

private:

    //-----------------------------------------------------
    RSGGAWork<O> work_;                     ///< Temporary structures.
    std::vector<Solution> tests_;           ///< Temporary solutions.
    std::unique_ptr<RGAPromFitness> prom_;  ///< PROMETHEE kernel.
    std::vector<double> tmpCriteria_;       ///< Temporary criteria values.
    std::vector<const std::vector<double>*> tmpSols_;   ///< Temporary solutions.
    std::vector<double> tmpFi_;             ///< Temporary Fi.
};


//------------------------------------------------------------------------------
/**
 * The RSGGAChromo class provides a chromosome of the Similarity-based Grouping Genetic Algorithm (SGGA), a GGA
 * clustering objects from the similarities, the agreement ratios and the disagreement ratios between each pair of
 * objects (see RSGGAData).
 *
 * The chromosome computes three criteria (see RSGGAOperators::evaluate):
 * - Criterion 0, similarity "J".
 * - Criterion 1, agreement.
 * - Criterion 2, disagreement.
 *
 * The first two must be maximized and the last one minimized, typically with the PROMETHEE fitness:
 * @code
 * "fitness": {"type": "promethee", "criteria": [
 *     {"name": "similarity", "type": "maximize"},
 *     {"name": "agreement", "type": "maximize"},
 *     {"name": "disagreement", "type": "minimize"}]}
 * @endcode
 *
 * The optimization (see optimize, enabled by the "optimize" parameter of RGAInst) builds several solutions from the
 * chromosome by successively dividing or merging groups, improves them with a k-Means, and keeps the best one according
 * to PROMETHEE II. The local optimization (see localOptimize, enabled by the "local_optimize" parameter of the GGA)
 * tries to insert the objects not assigned into the existing groups.
 * @tparam O                     Class of the objects.
 * @tparam G                     Class of the groups (it must inherit from RSGGAGroup<O>).
 * @tparam T                     Class of the thread-dependent data (it must inherit from RSGGAThreadData<G, O>).
 * @short SGGA Chromosome.
 */
template<class O, class G = RSGGAGroup<O>, class T = RSGGAThreadData<G, O>>
    class RSGGAChromo : public RGGAChromo<G, O, T, RSGGAContext<O>>
{
    static_assert(std::is_base_of_v<RSGGAGroup<O>, G>, "G must inherit from RSGGAGroup<O>");
    static_assert(std::is_base_of_v<RSGGAThreadData<G, O>, T>, "T must inherit from RSGGAThreadData<G, O>");

public:

    /**
     * Class of the solutions.
     */
    using Groups = RGGAGroups<G, O>;

    /**
     * Class of the operators.
     */
    using Operators = RSGGAOperators<G, O>;

    /**
     * Compute the three criteria (see the class description).
     * @param logger             Logger.
     * @param data               Thread-dependent data.
     */
    void evaluate([[maybe_unused]] RLogger& logger, T& data) override
    {
        auto& criteria = data.tmpCriteria();
        Operators::evaluate(*this, criteria);
        for (std::size_t i = 0; i < criteria.size(); ++i)
            this->criterion(i, criteria[i]);
    }

    /**
     * Optimize the chromosome. If nb_div_chromos (n) is not null, 2n+1 solutions are built:
     * - A copy of the chromosome.
     * - n solutions where the group containing the two most dissimilar
     *   objects is divided (each one from the previous one).
     * - n solutions where the two groups with the most similar centroids are
     *   merged (each one from the previous one).
     *
     * A k-Means is applied on each solution, and the groups containing a single social object are emptied. The
     * solutions and the chromosome are then compared with PROMETHEE II, and the chromosome is replaced by the best one.
     * @param logger             Logger.
     * @param data               Thread-dependent data.
     * @param random             Random generator.
     */
    void optimize([[maybe_unused]] RLogger& logger, T& data, RGARandom& random) override
    {
        [[maybe_unused]] mLogModule("r.sgga");
        const auto& context = this->context();
        const auto nbDiv = context.nbDivChromos();
        if ((nbDiv == 0) || (this->nbUsed() == 0))
            return;
        auto& tests = data.tests();
        auto& work = data.work();

        // Build the solutions
        tests[0].assign(*this);
        const Groups* lastDiv = this;
        const Groups* lastMerge = this;
        for (std::size_t i = 1; i <= nbDiv; ++i)
        {
            auto& div = tests[2 * i - 1];
            div.assign(*lastDiv);
            Operators::divideWorstObjects(context, div, work);
            lastDiv = &div;
            auto& merge = tests[2 * i];
            merge.assign(*lastMerge);
            Operators::mergeBestGroups(context, merge, work);
            lastMerge = &merge;
        }

        // Improve and evaluate them
        for (auto& test : tests)
        {
            Operators::kMeans(context, test, work, random);
            Operators::emptyGroups(context, test, work, false);
            Operators::evaluate(test, test.criteria());
        }

        // Compare them with the chromosome
        auto& own = data.tmpCriteria();
        Operators::evaluate(*this, own);
        auto& sols = data.tmpSols();
        auto& fi = data.tmpFi();
        sols.clear();
        sols.push_back(&own);
        for (const auto& test : tests)
            sols.push_back(&test.criteria());
        data.prom().compute(logger, sols, fi);
        std::size_t best = 0;
        for (std::size_t i = 1; i < fi.size(); ++i)
            if (fi[i] > fi[best])
                best = i;
        if (best == 0)
            return;
        mLogDebug(logger, "Chromosome ", this->id(), " replaced by solution ", best - 1, " (", this->nbUsed(), " -> ", tests[best - 1].nbUsed(), " groups)");
        static_cast<Groups&>(*this) = tests[best - 1];
        this->computeOrd();
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
     * Local optimization: try to insert the objects not assigned into the existing groups (see
     * RSGGAOperators::localOptimize).
     * @param logger             Logger.
     * @param data               Thread-dependent data.
     * @param random             Random generator.
     */
    void localOptimize([[maybe_unused]] RLogger& logger, T& data, RGARandom& random) override
    {
        auto& objs = data.tmpObjs();
        this->notAssigned(objs);
        Operators::localOptimize(this->context(), *this, objs, data.work(), random);
    }

    /**
     * Create a group with the context.
     * @param id                 Identifier of the group.
     */
    [[nodiscard]] G newGroup(const std::size_t id) const override { return G(id, this->context()); }

private:

    /**
     * Get the value of a criterion (0 if not computed).
     * @param idx                Index of the criterion.
     */
    [[nodiscard]] double criterionValue(const std::size_t idx) const
    {
        return (idx < this->criteria().size()) ? this->criteria()[idx] : 0.0;
    }
};


//------------------------------------------------------------------------------
/**
 * Population of the Similarity-based Grouping Genetic Algorithm:
 * @code
 * evocomb::RSGGAMatrixData data(nbObjs);
 * // ... fill the data ...
 * evocomb::RSGGAPopulation<MyObj> population(32, evocomb::RSGGAContext<MyObj>(objs, data), config);
 * evocomb::RGAInst inst(logger, config);
 * inst.init(&population);
 * inst.run(100);
 * @endcode
 * @tparam O                     Class of the objects.
 */
template<class O>
    using RSGGAPopulation = RGGAPopulation<RSGGAChromo<O>>;


}  //------- End of namespace evocomb ----------------------------------------------
