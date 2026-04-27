/*

    EvoComb

    RGGAChromo.hpp

    Chromosome of the Grouping Genetic Algorithm - Header.

    Copyright 1998-2026 by Pascal Francq (pascal@francq.info).
    Copyright 1998-2008 by the Université Libre de Bruxelles (ULB).

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
#include <random>
#include <string>
#include <type_traits>
#include <vector>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "app/r_logger.hpp"
#include "ga/r_ga.hpp"
#include "ga/r_ga_chromo.hpp"
#include "gga/r_gga_context.hpp"
#include "gga/r_gga_groups.hpp"
#include "gga/r_gga_heuristic.hpp"
#include "tools/r_concat.hpp"


//------------------------------------------------------------------------------
namespace evocomb {
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// forward declaration
template<class C> class RGGAPopulation;


//------------------------------------------------------------------------------
/**
 * The RGGAThreadData class provides the base class for the thread-dependent data of the grouping genetic algorithm: the
 * heuristic (created by createHeuristic from the name given by the context) and a temporary vector of objects.
 *
 * A problem can derive from this class to add its own heuristics (by overriding createHeuristic) or temporary
 * structures. The constructor must keep the same parameters (see RGGAPopulation). Once constructed, the data are
 * initialized by init (which can call virtual methods).
 * @tparam G                     Class of the groups.
 * @tparam O                     Class of the objects.
 * @tparam Ctx                   Class of the context.
 * @short GGA thread-dependent data.
 */
template<class G, class O, class Ctx = RGGAContext<O>>
    class RGGAThreadData
{
public:

    /**
     * Class of the context.
     */
    using Context = Ctx;

    /**
     * Class of the heuristics.
     */
    using Heuristic = RGGAHeuristic<G, O, Ctx>;

    /**
     * Construct the data.
     * @param logger             Logger.
     * @param context            Context of the GGA.
     */
    RGGAThreadData([[maybe_unused]] RLogger& logger, const Ctx& context) : context_(context) {}

    /**
     * Destruct the data.
     */
    virtual ~RGGAThreadData() = default;

    /**
     * Initialize the data: the heuristic given by the context is created (with createHeuristic). A derived class
     * overriding this method must call it.
     * @param logger             Logger.
     * @exception RGAException   The heuristic is unknown.
     */
    virtual void init(RLogger& logger)
    {
        heuristic_ = createHeuristic(logger, context_.heuristic());
    }

    /**
     * Get the context.
     */
    [[nodiscard]] const Ctx& context() const { return context_; }

    /**
     * Get the heuristic (init must have been called).
     */
    [[nodiscard]] Heuristic& heuristic() { return *heuristic_; }

    /**
     * Get a temporary vector of objects (for example for a local optimization).
     */
    [[nodiscard]] std::vector<const O*>& tmpObjs() { return tmpObjs_; }

    //-----------------------------------------------------
    RGGAThreadData(const RGGAThreadData&) = delete;
    RGGAThreadData& operator=(const RGGAThreadData&) = delete;

protected:

    /**
     * Create a heuristic from its name. "first_fit" is known; a derived class can override the method to add other
     * heuristics.
     * @param logger             Logger.
     * @param name               Name of the heuristic.
     * @exception RGAException   The heuristic is unknown.
     */
    [[nodiscard]] virtual std::unique_ptr<Heuristic> createHeuristic([[maybe_unused]] RLogger& logger, const std::string& name)
    {
        mLogModule("r.gga");
        if (name == "first_fit")
            return std::make_unique<RGGAFirstFitHeuristic<G, O, Ctx>>(context_);
        mThrowRGAException("Unknown heuristic '", name, "'");
    }

private:

    //-----------------------------------------------------
    const Ctx& context_;                    ///< Context.
    std::unique_ptr<Heuristic> heuristic_;  ///< Heuristic.
    std::vector<const O*> tmpObjs_;         ///< Temporary objects.
};


//------------------------------------------------------------------------------
/**
 * The RGGAChromo class provides a chromosome of the grouping genetic algorithm (GGA): a solution is a set of groups of
 * objects (see RGGAGroups), and the genetic operators work on the groups.
 *
 * - The random construction assigns all the objects with the construction
 *   heuristic.
 * - The crossover (BPX) inserts in the child some groups of the first parent,
 *   and the groups of the second parent that have no common objects with
 *   them (or only their objects that are not already assigned, see
 *   RGGAContext::emptyModifiedGroups). The objects not assigned are
 *   then assigned by the repair heuristic.
 * - The mutation releases some random groups and reassigns their objects
 *   with the repair heuristic.
 * - The inversion exchanges the positions of two groups.
 * - Before the repair heuristic, a local optimization can be done (see
 *   localOptimize).
 *
 * A problem defines its class of chromosome and implements at least evaluate:
 * @code
 * class MyThreadData : public RGGAThreadData<MyGroup, MyObj> { ... };
 *
 * class MyChromo : public RGGAChromo<MyGroup, MyObj, MyThreadData>
 * {
 * public:
 *     void evaluate(RLogger& logger, MyThreadData& data) override;
 * };
 * @endcode
 * The heuristics are created by the thread-dependent data (see RGGAThreadData::createHeuristic).
 * @tparam G                     Class of the groups.
 * @tparam O                     Class of the objects.
 * @tparam T                     Class of the thread-dependent data.
 * @tparam Ctx                   Class of the context (it must inherit from RGGAContext<O>).
 * @short GGA chromosome.
 */
template<class G, class O, class T, class Ctx = RGGAContext<O>>
    class RGGAChromo : public RGAChromo<T>, public RGGAGroups<G, O>
{
    static_assert(std::is_base_of_v<RGGAContext<O>, Ctx>, "Ctx must inherit from RGGAContext<O>");
    static_assert(std::is_base_of_v<RGGAThreadData<G, O, Ctx>, T>, "T must inherit from RGGAThreadData<G, O, Ctx>");

public:

    /**
     * Class of the context.
     */
    using Context = Ctx;

    /**
     * Class of the heuristics.
     */
    using Heuristic = RGGAHeuristic<G, O, Ctx>;

    /**
     * Get the context (it is available once the population is initialized).
     */
    [[nodiscard]] const Ctx& context() const { return *context_; }

    /**
     * Construct randomly a solution with the construction heuristic.
     * @param logger             Logger.
     * @param data               Thread-dependent data.
     * @param random             Random generator.
     */
    void randomConstruct(RLogger& logger, T& data, RGARandom& random) override
    {
        this->clearGroups();
        constructionHeuristic(data).run(logger, *this, random);
        this->computeOrd();
    }

    /**
     * Do a crossover (BPX) with the chromosome as child.
     *
     * Two crossing sites are chosen in the groups of each parent. The child receives the selected groups of the first
     * parent, and the groups of the second parent that are compatible with them (see RGGAGroup::isCompatible) and, if
     * emptyModifiedGroups is true, that have no common objects with them. If emptyModifiedGroups is false, only the
     * objects of these groups that are not in the selected groups are inserted. The objects not assigned are then
     * assigned by the repair heuristic.
     * @param logger             Logger.
     * @param data               Thread-dependent data.
     * @param random             Random generator.
     * @param parent1            First parent.
     * @param parent2            Second parent.
     * @exception RGAException   Some objects are not assigned by the heuristic.
     */
    void crossover(RLogger& logger, T& data, RGARandom& random, const RGGAChromo& parent1, const RGGAChromo& parent2)
    {
        mLogModule("r.gga");
        this->clearGroups();
        const auto nb1 = parent1.nbUsed();
        const auto nb2 = parent2.nbUsed();
        if (nb1 == 0 || nb2 == 0)
            mThrowRGAException("Crossover with a parent having no group");

        // Select the crossing sites: 'end' groups of parent1 from pos1 (and, if they go beyond the last group, the
        // 'begin' first groups).
        const auto pos1 = rand(random, nb1);
        auto end = nb1 > 1 ? rand(random, nb1 - 1) + 1 : 1;
        std::size_t begin = 0;
        if (end > nb1 - pos1)
        {
            begin = end - (nb1 - pos1);
            end = nb1 - pos1;
        }
        const auto pos2 = rand(random, nb2);

        // Groups of parent1 selected
        selected_.assign(parent1.nbGroupSlots(), 0);
        for (std::size_t i = 0; i < begin; ++i)
            selected_[parent1.used()[i]] = 1;
        for (std::size_t i = pos1; i < pos1 + end; ++i)
            selected_[parent1.used()[i]] = 1;

        // Groups of parent2 before pos2, then the selected groups of parent1, then the groups of parent2 from pos2.
        copyGroups(parent1, parent2, 0, pos2);
        for (std::size_t i = 0; i < begin; ++i)
            this->insertObjs(parent1.usedGroup(i), this->reserveGroup());
        for (std::size_t i = pos1; i < pos1 + end; ++i)
            this->insertObjs(parent1.usedGroup(i), this->reserveGroup());
        copyGroups(parent1, parent2, pos2, nb2 - pos2);

        repair(logger, data, random);
        this->computeOrd();
    }

    /**
     * Do a mutation: some random groups are released (a fifth of them if there are more than 10 groups, 2 if there are
     * more than 5, 1 otherwise), and their objects are reassigned by the repair heuristic.
     * @param logger             Logger.
     * @param data               Thread-dependent data.
     * @param random             Random generator.
     */
    void mutation(RLogger& logger, T& data, RGARandom& random) override
    {
        const auto nbUsed = this->nbUsed();
        std::size_t nb = 1;
        if (nbUsed > 10)
            nb = nbUsed / 5;
        else if (nbUsed > 5)
            nb = 2;
        for (std::size_t i = 0; i < nb && this->nbUsed() > 0; ++i)
            this->releaseGroup(this->used()[rand(random, this->nbUsed())]);
        repair(logger, data, random);
        this->computeOrd();
    }

    /**
     * Do an inversion: the positions of two random groups are exchanged (nothing is done with less than 3 groups).
     * @param logger             Logger.
     * @param data               Thread-dependent data.
     * @param random             Random generator.
     */
    void inversion([[maybe_unused]] RLogger& logger, [[maybe_unused]] T& data, RGARandom& random) override
    {
        const auto nbUsed = this->nbUsed();
        if (nbUsed < 3)
            return;
        const auto g1 = rand(random, nbUsed);
        auto g2 = rand(random, nbUsed - 1);
        if (g2 >= g1)
            ++g2;
        this->exchangeUsed(g1, g2);
    }

    /**
     * Verify the chromosome: the coherence of the groups, and at least one group must be used.
     * @param logger             Logger.
     * @param data               Thread-dependent data.
     * @exception RGAException   The chromosome is invalid.
     */
    void verify([[maybe_unused]] RLogger& logger, [[maybe_unused]] T& data) const override
    {
        mLogModule("r.gga");
        this->verifyGroups();
        if (this->nbUsed() == 0)
            mThrowRGAException("Chromosome ", this->id(), " has no group");
    }

    /**
     * Look if two chromosomes represent the same grouping (used by the "handle_duplicates" parameter of RGAInst).
     * @param chromo             Chromosome.
     */
    [[nodiscard]] bool isSame(const RGGAChromo& chromo) const { return this->sameGrouping(chromo); }

    /**
     * Get the number of group slots (reserved or free).
     */
    [[nodiscard]] std::size_t nbGroupSlots() const
    {
        std::size_t nb = 0;
        for (const auto id : this->used())
            if (id + 1 > nb)
                nb = id + 1;
        return nb;
    }

protected:

    /**
     * Get the heuristic used for the random construction. By default, it is the heuristic of the thread-dependent data.
     * @param data               Thread-dependent data.
     */
    [[nodiscard]] virtual Heuristic& constructionHeuristic(T& data) { return data.heuristic(); }

    /**
     * Get the heuristic used to assign the objects not assigned after a crossover or a mutation. By default, it is the
     * heuristic of the thread-dependent data.
     * @param data               Thread-dependent data.
     */
    [[nodiscard]] virtual Heuristic& repairHeuristic(T& data) { return data.heuristic(); }

    /**
     * Perform a local optimization before the repair heuristic, for example by trying to insert objects not assigned
     * into the existing groups (if enabled in the context). By default, nothing is done.
     * @param logger             Logger.
     * @param data               Thread-dependent data.
     * @param random             Random generator.
     */
    virtual void localOptimize([[maybe_unused]] RLogger& logger, [[maybe_unused]] T& data, [[maybe_unused]] RGARandom& random) {}

    /**
     * Assign the objects not assigned: local optimization (if enabled) and repair heuristic.
     * @param logger             Logger.
     * @param data               Thread-dependent data.
     * @param random             Random generator.
     * @exception RGAException   Some objects are not assigned.
     */
    void repair(RLogger& logger, T& data, RGARandom& random)
    {
        mLogModule("r.gga");
        if (context().localOptimize() && this->nbNotAssigned() > 0)
            localOptimize(logger, data, random);
        if (this->nbNotAssigned() > 0)
            repairHeuristic(data).run(logger, *this, random);
        if (this->nbNotAssigned() > 0)
            mThrowRGAException(this->nbNotAssigned(), " object(s) not assigned after the heuristic");
    }

    /**
     * Get a random number in [0, max[.
     * @param random             Random generator.
     * @param max                Upper bound (strictly positive).
     */
    [[nodiscard]] static std::size_t rand(RGARandom& random, const std::size_t max)
    {
        std::uniform_int_distribution<std::size_t> dist(0, max - 1);
        return dist(random);
    }

private:

    /**
     * Copy some groups of parent2 (see crossover).
     * @param parent1            First parent (its selected groups are in selected_).
     * @param parent2            Second parent.
     * @param pos2               Position of the first group of parent2.
     * @param nb2                Number of groups of parent2 to copy.
     */
    void copyGroups(const RGGAChromo& parent1, const RGGAChromo& parent2, const std::size_t pos2, const std::size_t nb2)
    {
        const auto emptyModified = context().emptyModifiedGroups();
        const auto isSelected = [&](const std::size_t objId) {
            const auto id = parent1.groupId(objId);
            return id != cNoRef && id < selected_.size() && selected_[id];
        };
        for (std::size_t i = pos2; i < pos2 + nb2; ++i)
        {
            const auto& src = parent2.usedGroup(i);

            // The group is not inserted if it is incompatible with a selected group or, if emptyModified, if it has
            // common objects with them.
            bool insert = true;
            for (const auto id : parent1.used())
                if (selected_[id] && !parent1.group(id).isCompatible(src))
                {
                    insert = false;
                    break;
                }
            if (insert && emptyModified)
                for (const auto obj : src.objs())
                    if (isSelected(obj->id()))
                    {
                        insert = false;
                        break;
                    }
            if (!insert)
                continue;

            const auto grp = this->reserveGroup();
            if (emptyModified)
                this->insertObjs(src, grp);
            else
            {
                // Only the objects not in the selected groups
                for (const auto obj : src.objs())
                    if (!isSelected(obj->id()))
                        this->insertObj(grp, *obj);
                if (this->group(grp).nbObjs() == 0)
                    this->releaseGroup(grp);
            }
        }
    }

    /**
     * Set the context (called by the population).
     * @param context            Context.
     */
    void setContext(const Ctx& context)
    {
        context_ = &context;
        this->initGroups(context.objs());
    }

    //-----------------------------------------------------
    const Ctx* context_ = nullptr;          ///< Context (shared).
    std::vector<char> selected_;            ///< Groups of parent1 selected (crossover).

    //-----------------------------------------------------
    template<class> friend class RGGAPopulation;
};


}  //------- End of namespace evocomb ----------------------------------------------
