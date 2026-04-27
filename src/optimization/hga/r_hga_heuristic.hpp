/*

    EvoComb

    RHGAHeuristic.hpp

    Heuristics of the Hierarchical Genetic Algorithm - Header.

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
#include <string>
#include <utility>
#include <vector>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "app/r_logger.hpp"
#include "ga/r_ga.hpp"
#include "hga/r_hga_attr_list.hpp"
#include "hga/r_hga_context.hpp"
#include "hga/r_hga_tree.hpp"
#include "tools/r_concat.hpp"
#include "tools/r_std.hpp"


//------------------------------------------------------------------------------
namespace evocomb {
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
 * The RHGAHeuristic class provides the base class for the heuristics attaching the objects not yet attached of a
 * solution (a tree, see RHGATree). A heuristic has access to the context of the HGA.
 *
 * The heuristic first builds the order in which the objects are treated (buildOrder, by default a random order), and
 * then attaches each object to the node returned by findNode (which can create new nodes).
 *
 * A heuristic has an internal state: an instance must not be used by two threads at the same time (each thread has its
 * own instances, see RHGAThreadData).
 * @tparam N                     Class of the nodes.
 * @tparam O                     Class of the objects.
 * @tparam Ctx                   Class of the context.
 * @short HGA Heuristic.
 */
template<class N, class O, class Ctx = RHGAContext<O>>
    class RHGAHeuristic
{
public:

    /**
     * Class of the solutions.
     */
    using Tree = RHGATree<N, O>;

    /**
     * Construct the heuristic.
     * @param name               Name of the heuristic.
     * @param context            Context of the HGA (it must live as long as the heuristic).
     */
    RHGAHeuristic(std::string name, const Ctx& context) : name_(std::move(name)), context_(context) {}

    /**
     * Destruct the heuristic.
     */
    virtual ~RHGAHeuristic() = default;

    /**
     * Get the name of the heuristic.
     */
    [[nodiscard]] const std::string& name() const { return name_; }

    /**
     * Get the context.
     */
    [[nodiscard]] const Ctx& context() const { return context_; }

    /**
     * Attach all the objects not yet attached of a solution.
     * @param logger             Logger.
     * @param tree               Solution.
     * @param random             Random generator.
     * @exception RGAException   No node was found for an object.
     */
    void run(RLogger& logger, Tree& tree, RGARandom& random)
    {
        mLogModule("r.hga");
        buildOrder(logger, tree, random);
        for (const auto obj : order_)
        {
            const auto id = findNode(logger, tree, *obj, random);
            if (id == cNoRef)
                mThrowRGAException(name_, ": no node found for object ", obj->id());
            tree.insertObj(id, *obj);
        }
        postRun(logger, tree, random);
    }

    //-----------------------------------------------------
    RHGAHeuristic(const RHGAHeuristic&) = delete;
    RHGAHeuristic& operator=(const RHGAHeuristic&) = delete;

protected:

    /**
     * Build the order in which the objects not attached are treated (in the vector order_). By default, the objects are
     * randomly ordered.
     * @param logger             Logger.
     * @param tree               Solution.
     * @param random             Random generator.
     */
    virtual void buildOrder([[maybe_unused]] RLogger& logger, Tree& tree, RGARandom& random)
    {
        tree.notAssigned(order_);
        std::shuffle(order_.begin(), order_.end(), random);
    }

    /**
     * Find a node for an object. The method can create new nodes.
     * @param logger             Logger.
     * @param tree               Solution.
     * @param obj                Object.
     * @param random             Random generator.
     * @return the identifier of the node (cNoRef if no node can be found).
     */
    [[nodiscard]] virtual std::size_t findNode(RLogger& logger, Tree& tree, const O& obj, RGARandom& random) = 0;

    /**
     * Method called once all the objects are attached.
     * @param logger             Logger.
     * @param tree               Solution.
     * @param random             Random generator.
     */
    virtual void postRun([[maybe_unused]] RLogger& logger, [[maybe_unused]] Tree& tree, [[maybe_unused]] RGARandom& random) {}

    //-----------------------------------------------------
    std::vector<const O*> order_;           ///< Order of the objects to attach.

private:

    //-----------------------------------------------------
    std::string name_;                      ///< Name of the heuristic.
    const Ctx& context_;                    ///< Context.
};


//------------------------------------------------------------------------------
/**
 * The RHGAFirstNodeHeuristic class provides the "first node" heuristic. Starting from the top of the tree, it searches,
 * level by level (the nodes of a level being examined in a random order), the branch where the object can be attached:
 * 1. If a node has exactly the attributes of the object, the object is
 *    attached to it.
 * 2. If the attributes of a node are all attributes of the object, the search
 *    continues with its child nodes.
 * 3. Otherwise, the node sharing the most attributes with the object is
 *    considered. If it shares no attribute, or only those of the current
 *    parent, a new node with the attributes of the object is created under
 *    the current parent. Otherwise, an intermediate node with the common
 *    attributes is created, the node becomes its child, and the object is
 *    attached to the intermediate node (if it has exactly its attributes) or
 *    to a new child node of it.
 * @tparam N                     Class of the nodes.
 * @tparam O                     Class of the objects.
 * @tparam Ctx                   Class of the context.
 * @short HGA First Node Heuristic.
 */
template<class N, class O, class Ctx = RHGAContext<O>>
    class RHGAFirstNodeHeuristic : public RHGAHeuristic<N, O, Ctx>
{
public:

    /**
     * Class of the solutions.
     */
    using Tree = RHGATree<N, O>;

    /**
     * Construct the heuristic.
     * @param context            Context of the HGA.
     * @param name               Name of the heuristic.
     */
    explicit RHGAFirstNodeHeuristic(const Ctx& context, std::string name = "first_node") :
        RHGAHeuristic<N, O, Ctx>(std::move(name), context)
    {
    }

protected:

    /**
     * Find a node for an object (see the class description).
     * @param logger             Logger.
     * @param tree               Solution.
     * @param obj                Object.
     * @param random             Random generator.
     */
    [[nodiscard]] std::size_t findNode([[maybe_unused]] RLogger& logger, Tree& tree, const O& obj, RGARandom& random) override
    {
        const auto& attrs = obj.attrs();
        std::size_t parent = cNoRef;
        while (true)
        {
            // Examine the nodes of the current level in a random order
            level_ = tree.children(parent);
            std::shuffle(level_.begin(), level_.end(), random);
            std::size_t best = cNoRef;
            std::size_t bestNb = 0;
            std::size_t deeper = cNoRef;
            for (const auto id : level_)
            {
                const auto& nodeAttrs = tree.node(id).attrs();
                const auto nb = nodeAttrs.nbCommon(attrs);
                if (nb == 0)
                    continue;
                if (nb == nodeAttrs.size())
                {
                    // The node has exactly the attributes of the object, or the search continues in its branch
                    if (nb == attrs.size())
                        return id;
                    deeper = id;
                    break;
                }
                if (nb > bestNb)
                {
                    bestNb = nb;
                    best = id;
                }
            }
            if (deeper != cNoRef)
            {
                parent = deeper;
                continue;
            }

            // No node shares attributes -> new node
            if (best == cNoRef)
                return tree.addNode(parent, attrs);

            // The common attributes are those of the parent -> new node
            const auto common = RHGAAttrList::inter(attrs, tree.node(best).attrs());
            if ((parent != cNoRef) && (common.size() == tree.node(parent).attrs().size()))
                return tree.addNode(parent, attrs);

            // Intermediate node with the common attributes
            const auto inter = tree.addNode(parent, common);
            tree.moveNode(inter, best);
            if (common.size() == attrs.size())
                return inter;
            return tree.addNode(inter, attrs);
        }
    }

private:

    //-----------------------------------------------------
    std::vector<std::size_t> level_;        ///< Nodes of the current level.
};


}  //------- End of namespace evocomb ----------------------------------------------
