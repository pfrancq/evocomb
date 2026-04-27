/*

    EvoComb

    RHGANode.hpp

    Node of the Hierarchical Genetic Algorithm - Header.

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
#include "hga/r_hga_attr_list.hpp"
#include "tools/r_std.hpp"


//------------------------------------------------------------------------------
namespace evocomb {
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// forward declaration
template<class N, class O> class RHGATree;


//------------------------------------------------------------------------------
/**
 * The RHGANode class provides the base class for the nodes of the tree built by the Hierarchical Genetic Algorithm (see
 * RHGATree). A node is described by a list of attributes, has child nodes, and objects attached to it.
 *
 * A node is a value: it is copied with the tree containing it. The child nodes and the parent are referenced by their
 * identifiers. The node does not know its tree: the nodes and the objects are inserted and deleted through the tree,
 * which calls the hooks of the node (postInsertNode, postInsertObj, etc.). A derived class overrides these hooks to
 * maintain its own information.
 * @tparam O                     Class of the objects.
 * @short HGA Node.
 */
template<class O>
    class RHGANode
{
public:

    /**
     * Construct the node.
     * @param id                 Identifier of the node.
     */
    explicit RHGANode(const std::size_t id = cNoRef) : id_(id) {}

    RHGANode(const RHGANode&) = default;
    RHGANode(RHGANode&&) noexcept = default;
    RHGANode& operator=(const RHGANode&) = default;
    RHGANode& operator=(RHGANode&&) noexcept = default;

    /**
     * Destruct the node.
     */
    virtual ~RHGANode() = default;

    /**
     * Get the identifier of the node.
     */
    [[nodiscard]] std::size_t id() const { return id_; }

    /**
     * Look if the node is used in the tree.
     */
    [[nodiscard]] bool isReserved() const { return reserved_; }

    /**
     * Get the identifier of the parent node (cNoRef for a top node).
     */
    [[nodiscard]] std::size_t parent() const { return parent_; }

    /**
     * Get the identifiers of the child nodes.
     */
    [[nodiscard]] const std::vector<std::size_t>& children() const { return children_; }

    /**
     * Get the number of child nodes.
     */
    [[nodiscard]] std::size_t nbNodes() const { return children_.size(); }

    /**
     * Get the objects attached to the node.
     */
    [[nodiscard]] const std::vector<const O*>& objs() const { return objs_; }

    /**
     * Get the number of objects attached to the node.
     */
    [[nodiscard]] std::size_t nbObjs() const { return objs_.size(); }

    /**
     * Get the attributes of the node.
     */
    [[nodiscard]] const RHGAAttrList& attrs() const { return attrs_; }

    /**
     * Method called after a child node was inserted.
     * @param id                 Identifier of the child node.
     */
    virtual void postInsertNode([[maybe_unused]] std::size_t id) {}

    /**
     * Method called after a child node was deleted (or moved elsewhere).
     * @param id                 Identifier of the child node.
     */
    virtual void postDeleteNode([[maybe_unused]] std::size_t id) {}

    /**
     * Method called after an object was attached.
     * @param obj                Object.
     */
    virtual void postInsertObj([[maybe_unused]] const O& obj) {}

    /**
     * Method called after an object was detached.
     * @param obj                Object.
     */
    virtual void postDeleteObj([[maybe_unused]] const O& obj) {}

    /**
     * Copy the specific information of a node of another tree (called when a tree is copied except a branch, see
     * RHGATree::copyExceptBranch). The attributes are already copied.
     * @param node               Node used as source.
     */
    virtual void copyInfos([[maybe_unused]] const RHGANode& node) {}

    /**
     * Method called when the node is released. It can be used to reset some information.
     */
    virtual void clear() {}

    /**
     * Verify the specific information of the node. An exception should be thrown if the node is invalid. The coherence
     * of the tree is verified by RHGATree.
     */
    virtual void verify() const {}

private:

    //-----------------------------------------------------
    std::size_t id_;                        ///< Identifier.
    bool reserved_ = false;                 ///< Is the node used?
    std::size_t parent_ = cNoRef;           ///< Parent node (cNoRef for a top node).
    std::vector<std::size_t> children_;     ///< Child nodes.
    std::vector<const O*> objs_;            ///< Objects attached.
    RHGAAttrList attrs_;                    ///< Attributes.

    //-----------------------------------------------------
    template<class, class> friend class RHGATree;
};


}  //------- End of namespace evocomb ----------------------------------------------
