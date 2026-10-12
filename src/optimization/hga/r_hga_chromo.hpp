/*

    EvoComb

    RHGAChromo.hpp

    Chromosome of the Hierarchical Genetic Algorithm - Header.

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
#include <algorithm>
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
#include "hga/r_hga_context.hpp"
#include "hga/r_hga_heuristic.hpp"
#include "hga/r_hga_tree.hpp"
#include "tools/r_concat.hpp"


//------------------------------------------------------------------------------
namespace evocomb {
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// forward declaration
template<class C> class RHGAPopulation;


//------------------------------------------------------------------------------
/**
 * The RHGAThreadData class provides the base class for the thread-dependent data of the Hierarchical Genetic Algorithm:
 * the heuristic (created by createHeuristic from the name given by the context) and some temporary vectors.
 *
 * A problem can derive from this class to add its own heuristics (by overriding createHeuristic) or temporary
 * structures. The constructor must keep the same parameters (see RHGAPopulation). Once constructed, the data are
 * initialized by init (which can call virtual methods).
 * @tparam N                     Class of the nodes.
 * @tparam O                     Class of the objects.
 * @tparam Ctx                   Class of the context.
 * @short HGA thread-dependent data.
 */
template<class N, class O, class Ctx = RHGAContext<O>>
    class RHGAThreadData
{
public:

    /**
     * Class of the context.
     */
    using Context = Ctx;

    /**
     * Class of the heuristics.
     */
    using Heuristic = RHGAHeuristic<N, O, Ctx>;

    /**
     * Construct the data.
     * @param logger             Logger.
     * @param context            Context of the HGA.
     */
    RHGAThreadData([[maybe_unused]] RLogger& logger, const Ctx& context) : context_(context) {}

    /**
     * Destruct the data.
     */
    virtual ~RHGAThreadData() = default;

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
     * Get a first temporary vector of node identifiers.
     */
    [[nodiscard]] std::vector<std::size_t>& tmpNodes1() { return tmpNodes1_; }

    /**
     * Get a second temporary vector of node identifiers.
     */
    [[nodiscard]] std::vector<std::size_t>& tmpNodes2() { return tmpNodes2_; }

    /**
     * Get a temporary vector of objects.
     */
    [[nodiscard]] std::vector<const O*>& tmpObjs() { return tmpObjs_; }

    /**
     * Get a temporary vector of flags (one per object).
     */
    [[nodiscard]] std::vector<char>& tmpFlags() { return tmpFlags_; }

    //-----------------------------------------------------
    RHGAThreadData(const RHGAThreadData&) = delete;
    RHGAThreadData& operator=(const RHGAThreadData&) = delete;

protected:

    /**
     * Create a heuristic from its name. "first_node" is known; a derived class can override the method to add other
     * heuristics.
     * @param logger             Logger.
     * @param name               Name of the heuristic.
     * @exception RGAException   The heuristic is unknown.
     */
    [[nodiscard]] virtual std::unique_ptr<Heuristic> createHeuristic([[maybe_unused]] RLogger& logger, const std::string& name)
    {
        mLogModule("r.hga");
        if (name == "first_node")
            return std::make_unique<RHGAFirstNodeHeuristic<N, O, Ctx>>(context_);
        mThrowRGAException("Unknown heuristic '", name, "'");
    }

private:

    //-----------------------------------------------------
    const Ctx& context_;                    ///< Context.
    std::unique_ptr<Heuristic> heuristic_;  ///< Heuristic.
    std::vector<std::size_t> tmpNodes1_;    ///< Temporary node identifiers.
    std::vector<std::size_t> tmpNodes2_;    ///< Temporary node identifiers.
    std::vector<const O*> tmpObjs_;         ///< Temporary objects.
    std::vector<char> tmpFlags_;            ///< Temporary flags.
};


//------------------------------------------------------------------------------
/**
 * The RHGAChromo class provides a chromosome of the Hierarchical Genetic Algorithm (HGA): a solution is a tree of nodes
 * described by attributes, to which the objects are attached (see RHGATree).
 * - The random construction attaches all the objects with the heuristic.
 * - The crossover chooses two nodes with the same attributes in the parents
 *   (the crossing nodes). The child is a copy of the first parent without
 *   the branch of its crossing node and without the objects of the branch of
 *   the crossing node of the second parent. This branch is then copied at
 *   the place of the removed one, and the objects not attached are attached
 *   by the heuristic.
 * - The mutation deletes a random node (with its branch), and the objects not
 *   attached are attached by the heuristic.
 *
 * A problem defines its class of chromosome and implements at least evaluate:
 * @code
 * class MyChromo : public RHGAChromo<RHGANode<MyObj>, MyObj, RHGAThreadData<RHGANode<MyObj>, MyObj>>
 * {
 * public:
 *     void evaluate(RLogger& logger, ThreadData& data) override;
 * };
 * @endcode
 * @tparam N                     Class of the nodes.
 * @tparam O                     Class of the objects.
 * @tparam T                     Class of the thread-dependent data.
 * @tparam Ctx                   Class of the context (it must inherit from RHGAContext<O>).
 * @short HGA chromosome.
 */
template<class N, class O, class T, class Ctx = RHGAContext<O>>
    class RHGAChromo : public RGAChromo<T>, public RHGATree<N, O>
{
    static_assert(std::is_base_of_v<RHGAContext<O>, Ctx>, "Ctx must inherit from RHGAContext<O>");
    static_assert(std::is_base_of_v<RHGAThreadData<N, O, Ctx>, T>, "T must inherit from RHGAThreadData<N, O, Ctx>");

public:

    /**
     * Class of the context.
     */
    using Context = Ctx;

    /**
     * Class of the solutions.
     */
    using Tree = RHGATree<N, O>;

    /**
     * Get the context (it is available once the population is initialized).
     */
    [[nodiscard]] const Ctx& context() const { return *context_; }

    /**
     * Construct randomly a solution with the heuristic.
     * @param logger             Logger.
     * @param data               Thread-dependent data.
     * @param random             Random generator.
     */
    void randomConstruct(RLogger& logger, T& data, RGARandom& random) override
    {
        this->clearTree();
        data.heuristic().run(logger, *this, random);
        this->computeCanonical();
    }

    /**
     * Do a crossover with the chromosome as child (see the class description). If the parents have no nodes with the
     * same attributes, the child is a copy of the first parent on which a mutation is done.
     * @param logger             Logger.
     * @param data               Thread-dependent data.
     * @param random             Random generator.
     * @param parent1            First parent.
     * @param parent2            Second parent.
     */
    void crossover(RLogger& logger, T& data, RGARandom& random, const RHGAChromo& parent1, const RHGAChromo& parent2)
    {
        [[maybe_unused]] mLogModule("r.hga");

        // Choose randomly two nodes with the same attributes
        auto& nodes1 = data.tmpNodes1();
        auto& nodes2 = data.tmpNodes2();
        nodes1 = parent1.used();
        nodes2 = parent2.used();
        std::shuffle(nodes1.begin(), nodes1.end(), random);
        std::shuffle(nodes2.begin(), nodes2.end(), random);
        std::size_t node1 = cNoRef;
        std::size_t node2 = cNoRef;
        for (std::size_t i = 0; (i < nodes1.size()) && (node1 == cNoRef); ++i)
            for (std::size_t j = 0; j < nodes2.size(); ++j)
                if (parent1.node(nodes1[i]).attrs() == parent2.node(nodes2[j]).attrs())
                {
                    node1 = nodes1[i];
                    node2 = nodes2[j];
                    break;
                }
        if (node1 == cNoRef)
        {
            mLogDebug(logger, "No crossing nodes between chromosomes ", parent1.id(), " and ", parent2.id());
            static_cast<Tree&>(*this) = parent1;
            mutation(logger, data, random);
            return;
        }
        mLogDebug(logger, "Crossing nodes ", node1, " (chromosome ", parent1.id(), ") and ", node2, " (chromosome ", parent2.id(), ") with attributes ", parent1.node(node1).attrs().toString());

        // Objects of the branch of node2
        auto& objs = data.tmpObjs();
        objs.clear();
        parent2.allObjs(node2, objs);
        auto& skip = data.tmpFlags();
        skip.assign(this->nbObjs(), 0);
        for (const auto obj : objs)
            skip[obj->id()] = 1;

        // Copy parent1 without the branch of node1 and the objects of the branch of node2, then copy the branch of
        // node2 at the place of node1
        const auto parent = this->copyExceptBranch(parent1, node1, &skip);
        this->copyBranch(parent, parent2, node2);

        // Attach the objects not attached
        repair(logger, data, random);
        this->computeCanonical();
    }

    /**
     * Do a mutation: a random node is deleted (with its branch), and the objects not attached are attached by the
     * heuristic.
     * @param logger             Logger.
     * @param data               Thread-dependent data.
     * @param random             Random generator.
     */
    void mutation(RLogger& logger, T& data, RGARandom& random) override
    {
        [[maybe_unused]] mLogModule("r.hga");
        if (this->nbNodes() > 0)
        {
            std::uniform_int_distribution<std::size_t> dist(0, this->nbNodes() - 1);
            const auto id = this->used()[dist(random)];
            mLogDebug(logger, "Mutation of chromosome ", this->id(), ": node ", id, " deleted");
            this->deleteNode(id);
            this->cleanUp();
        }
        repair(logger, data, random);
        this->computeCanonical();
    }

    /**
     * Verify the chromosome: the coherence of the tree (all the objects must be attached).
     * @param logger             Logger.
     * @param data               Thread-dependent data.
     * @exception RGAException   The chromosome is invalid.
     */
    void verify([[maybe_unused]] RLogger& logger, [[maybe_unused]] T& data) const override
    {
        this->verifyTree(true);
    }

    /**
     * Look if two chromosomes represent the same tree (used by the "handle_duplicates" parameter of RGAInst).
     * @param chromo             Chromosome.
     */
    [[nodiscard]] bool isSame(const RHGAChromo& chromo) const { return this->sameTree(chromo); }

protected:

    /**
     * Attach the objects not attached with the heuristic.
     * @param logger             Logger.
     * @param data               Thread-dependent data.
     * @param random             Random generator.
     * @exception RGAException   Some objects are not attached.
     */
    void repair(RLogger& logger, T& data, RGARandom& random)
    {
        mLogModule("r.hga");
        if (this->nbNotAssigned() > 0)
            data.heuristic().run(logger, *this, random);
        if (this->nbNotAssigned() > 0)
            mThrowRGAException(this->nbNotAssigned(), " object(s) not attached after the heuristic");
    }

private:

    /**
     * Set the context (called by the population).
     * @param context            Context.
     */
    void setContext(const Ctx& context)
    {
        context_ = &context;
        this->initTree(context.objs());
    }

    //-----------------------------------------------------
    const Ctx* context_ = nullptr;          ///< Context (shared).

    //-----------------------------------------------------
    template<class> friend class RHGAPopulation;
};


}  //------- End of namespace evocomb ----------------------------------------------
