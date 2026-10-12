/*

    EvoComb - Bin Packing example

    RChromoBP.hpp

    Grouping Genetic Algorithm for the Bin Packing Problem - Header.

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
#include <algorithm>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "gga/r_gga_chromo.hpp"
#include "gga/r_gga_heuristic.hpp"
#include "gga/r_gga_population.hpp"
#include "r_bp_problem.hpp"
#include "tools/r_json.hpp"


//------------------------------------------------------------------------------
namespace evocomb::bp {
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
 * The RContextBP class provides the context of the GGA for the bin packing:
 * the objects, the parameters of the GGA and the capacity of the bins.
 * @short Bin Packing GGA Context.
 */
class RContextBP : public RGGAContext<RObjBP>
{
public:

    /**
     * Construct the context from a problem.
     * @param problem            Problem (it must live as long as the context).
     */
    explicit RContextBP(const RBPProblem& problem) :
        RGGAContext<RObjBP>(problem.objPtrs()),
        capacity_(problem.capacity())
    {
    }

    /**
     * Get the capacity of the bins.
     */
    [[nodiscard]] std::size_t capacity() const { return capacity_; }

private:

    //-----------------------------------------------------
    std::size_t capacity_;                  ///< Capacity of the bins.
};


//------------------------------------------------------------------------------
/**
 * The RGroupBP class provides a bin: a group whose total size of the objects cannot exceed its capacity.
 * @short Bin.
 */
class RGroupBP : public RGGAGroup<RObjBP>
{
public:

    /**
     * Construct the bin.
     * @param id                 Identifier.
     * @param capacity           Capacity.
     */
    RGroupBP(std::size_t id, std::size_t capacity) : RGGAGroup<RObjBP>(id), capacity_(capacity) {}

    /**
     * Get the total size of the objects.
     */
    [[nodiscard]] std::size_t size() const { return size_; }

    /**
     * Get the capacity.
     */
    [[nodiscard]] std::size_t capacity() const { return capacity_; }

    /**
     * Get the filling ratio (size / capacity).
     */
    [[nodiscard]] double ratio() const { return static_cast<double>(size_) / static_cast<double>(capacity_); }

    /**
     * An object can be inserted if the capacity is not exceeded.
     * @param obj                Object.
     */
    [[nodiscard]] bool canInsert(const RObjBP& obj) const override { return size_ + obj.size() <= capacity_; }

    /**
     * Update the size after an insertion.
     * @param obj                Object.
     */
    void postInsert(const RObjBP& obj) override { size_ += obj.size(); }

    /**
     * Update the size after a deletion.
     * @param obj                Object.
     */
    void postDelete(const RObjBP& obj) override { size_ -= obj.size(); }

    /**
     * Reset the size.
     */
    void clear() override { size_ = 0; }

    /**
     * Verify that the capacity is not exceeded.
     * @exception RGAException   The capacity is exceeded.
     */
    void verify() const override;

private:

    //-----------------------------------------------------
    std::size_t capacity_;                  ///< Capacity.
    std::size_t size_ = 0;                  ///< Total size of the objects.
};


//------------------------------------------------------------------------------
/**
 * The RFirstFitDescendingHeuristic class provides the first-fit descending heuristic: the objects are treated by
 * decreasing size (the objects with the same size in a random order), each one being put in the first bin that can
 * contain it.
 * @tparam G                     Class of the groups.
 * @tparam O                     Class of the objects (they must provide a method size()).
 * @tparam Ctx                   Class of the context.
 * @short First-Fit Descending Heuristic.
 */
template<class G, class O, class Ctx>
    class RFirstFitDescendingHeuristic : public RGGAFirstFitHeuristic<G, O, Ctx>
{
public:

    /**
     * Class of the solutions.
     */
    using Groups = RGGAGroups<G, O>;

    /**
     * Construct the heuristic.
     * @param context            Context of the GGA.
     */
    explicit RFirstFitDescendingHeuristic(const Ctx& context) : RGGAFirstFitHeuristic<G, O, Ctx>(context, "first_fit_descending") {}

protected:

    /**
     * Order the objects not assigned by decreasing size.
     * @param logger             Logger.
     * @param groups             Solution.
     * @param random             Random generator.
     */
    void buildOrder(RLogger& logger, Groups& groups, RGARandom& random) override
    {
        RGGAHeuristic<G, O, Ctx>::buildOrder(logger, groups, random);
        std::stable_sort(this->order_.begin(), this->order_.end(), [](const auto* a, const auto* b) { return a->size() > b->size(); });
    }
};


//------------------------------------------------------------------------------
/**
 * The RThreadDataBP class provides the thread-dependent data of the GGA for the bin packing: the heuristics and
 * temporary vectors.
 * @short Bin Packing Thread-dependent Data.
 */
class RThreadDataBP : public RGGAThreadData<RGroupBP, RObjBP, RContextBP>
{
public:

    /**
     * First-fit descending heuristic.
     */
    using FirstFitDescending = RFirstFitDescendingHeuristic<RGroupBP, RObjBP, RContextBP>;

    /**
     * Construct the data.
     * @param logger             Logger.
     * @param context            Context.
     */
    RThreadDataBP(RLogger& logger, const RContextBP& context) :
        RGGAThreadData<RGroupBP, RObjBP, RContextBP>(logger, context),
        ffd_(std::make_unique<FirstFitDescending>(context))
    {
    }

    /**
     * Get the first-fit descending heuristic.
     */
    [[nodiscard]] Heuristic& ffd() { return *ffd_; }

    /**
     * Get a temporary vector for the objects of a bin.
     */
    [[nodiscard]] std::vector<const RObjBP*>& binObjs() { return binObjs_; }

    /**
     * Get a temporary vector for the objects to delete from a bin.
     */
    [[nodiscard]] std::vector<const RObjBP*>& delObjs() { return delObjs_; }

protected:

    /**
     * Create a heuristic from its name ("first_fit" or "first_fit_descending").
     * @param logger             Logger.
     * @param name               Name.
     * @exception RGAException   The heuristic is unknown.
     */
    [[nodiscard]] std::unique_ptr<Heuristic> createHeuristic(RLogger& logger, const std::string& name) override;

private:

    //-----------------------------------------------------
    std::unique_ptr<Heuristic> ffd_;  ///< First-fit descending heuristic.
    std::vector<const RObjBP*> binObjs_;    ///< Objects of a bin.
    std::vector<const RObjBP*> delObjs_;    ///< Objects to delete from a bin.
};


//------------------------------------------------------------------------------
/**
 * The RChromoBP class provides the chromosome of the GGA for the bin packing.
 *
 * - The random construction uses the heuristic of the configuration
 *   ("first_fit" by default, or "first_fit_descending", see
 *   RThreadDataBP).
 * - The crossover and the mutation use the first-fit descending heuristic
 *   to assign the objects not assigned. The mutation first releases the
 *   least filled bin.
 * - The local optimization tries to replace up to three objects of a bin by
 *   one or two objects not assigned, so that the bin is more filled (as in
 *   the BPRX operator of Falkenauer).
 * - The fitness (criterion 0) is the mean of the squared filling ratios of
 *   the bins, which favours solutions with few, well-filled bins.
 * @short Bin Packing Chromosome.
 */
class RChromoBP : public RGGAChromo<RGroupBP, RObjBP, RThreadDataBP, RContextBP>
{
public:

    /**
     * Compute the fitness (criterion 0): the mean of the squared filling ratios of the bins.
     * @param logger             Logger.
     * @param data               Thread-dependent data.
     */
    void evaluate(RLogger& logger, RThreadDataBP& data) override;

    /**
     * Release the least filled bin, then do the mutation of the GGA.
     * @param logger             Logger.
     * @param data               Thread-dependent data.
     * @param random             Random generator.
     */
    void mutation(RLogger& logger, RThreadDataBP& data, RGARandom& random) override;

    /**
     * Export the bins in JSON (one object per bin, which is written on a single line by an indented dump if it is short
     * enough):
     * @code
     * [
     *   {"size": 148, "objects": [3, 17], "sizes": [42, 106]},
     *   ...
     * ]
     * @endcode
     * "size" is the total size of the bin, "objects" the identifiers of its objects (their index in the problem) and
     * "sizes" their sizes.
     * @return a JSON array containing the bins.
     */
    [[nodiscard]] std::unique_ptr<RJSON> toJSON() const;

protected:

    /**
     * Create a bin with the capacity of the problem.
     * @param id                 Identifier.
     */
    [[nodiscard]] RGroupBP newGroup(std::size_t id) const override { return RGroupBP(id, context().capacity()); }

    /**
     * The repair uses the first-fit descending heuristic.
     * @param data               Thread-dependent data.
     */
    [[nodiscard]] Heuristic& repairHeuristic(RThreadDataBP& data) override;

    /**
     * Local optimization: for each bin, try to replace up to three of its objects by one or two objects not assigned,
     * so that the bin is more filled. It is repeated as long as a bin is improved.
     * @param logger             Logger.
     * @param data               Thread-dependent data.
     * @param random             Random generator.
     */
    void localOptimize(RLogger& logger, RThreadDataBP& data, RGARandom& random) override;

private:

    /**
     * Try to improve a bin with the objects not assigned (see localOptimize).
     * @param id                 Identifier of the bin.
     * @param free               Objects not assigned (sorted by decreasing size). It is updated.
     * @param data               Thread-dependent data.
     * @return `true` if the bin was improved.
     */
    bool optimizeBin(std::size_t id, std::vector<const RObjBP*>& free, RThreadDataBP& data);

    /**
     * Look if objects with a given total size can be added to a bin, after the deletion of up to three of its objects,
     * so that the bin is more filled.
     * @param bin                Bin.
     * @param add                Total size of the objects to add.
     * @param del                Objects to delete (filled by the method).
     * @param data               Thread-dependent data.
     */
    static bool canImprove(const RGroupBP& bin, std::size_t add, std::vector<const RObjBP*>& del, RThreadDataBP& data);
};


//------------------------------------------------------------------------------
/**
 * Population of the GGA for the bin packing.
 */
using RPopulationBP = RGGAPopulation<RChromoBP>;


}  //------- End of namespace evocomb::bp ------------------------------------------
