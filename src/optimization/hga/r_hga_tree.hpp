/*

    EvoComb

    RHGATree.hpp

    Tree of the Hierarchical Genetic Algorithm - Header.

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
#include <deque>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "app/r_logger.hpp"
#include "ga/r_ga.hpp"
#include "hga/r_hga_attr_list.hpp"
#include "hga/r_hga_node.hpp"
#include "tools/r_concat.hpp"
#include "tools/r_std.hpp"


//------------------------------------------------------------------------------
namespace evocomb {
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
 * The RHGATree class provides a tree of nodes to which objects are attached:
 * it is the representation of a solution of a hierarchical problem (see RHGAChromo). Each node has a list of
 * attributes, which includes the attributes of its parent, and the objects attached to a node have the same attributes
 * as it.
 *
 * The objects are identified by their index in a vector of objects shared by all the solutions (object i has the
 * identifier i). The nodes are identified by their index in the tree: a node is either used or free. The nodes and the
 * objects are always inserted, moved or deleted through the tree, which maintains the coherence of the structure and
 * calls the hooks of the nodes.
 *
 * The tree is a value: it can be copied (the objects are shared, not copied).
 * @tparam N                     Class of the nodes (it must inherit from RHGANode<O>).
 * @tparam O                     Class of the objects.
 * @short HGA Tree.
 */
template<class N, class O>
    class RHGATree
{
    static_assert(std::is_base_of_v<RHGANode<O>, N>, "N must inherit from RHGANode<O>");

public:

    /**
     * Class of the nodes.
     */
    using Node = N;

    /**
     * Class of the objects.
     */
    using Object = O;

    /**
     * Construct the tree.
     */
    RHGATree() = default;

    RHGATree(const RHGATree&) = default;
    RHGATree(RHGATree&&) noexcept = default;
    RHGATree& operator=(const RHGATree&) = default;
    RHGATree& operator=(RHGATree&&) noexcept = default;

    /**
     * Destruct the tree.
     */
    virtual ~RHGATree() = default;

    /**
     * Initialize the tree with the objects to place (the tree is emptied).
     * @param objs               Objects (objs[i]->id() must be i). The vector must live as long as the tree.
     */
    void initTree(const std::vector<const O*>& objs)
    {
        objs_ = &objs;
        nodes_.clear();
        clearTree();
    }

    /**
     * Remove all the nodes (no object is attached anymore).
     */
    void clearTree()
    {
        for (const auto id : used_)
            resetNode(nodes_[id]);
        used_.clear();
        top_.clear();
        objsAss_.assign(nbObjs(), cNoRef);
        nbAssigned_ = 0;
        canonical_.clear();
    }

    /**
     * Get the number of objects to place.
     */
    [[nodiscard]] std::size_t nbObjs() const { return objs_ ? objs_->size() : 0; }

    /**
     * Get the objects to place.
     */
    [[nodiscard]] const std::vector<const O*>& objs() const { return *objs_; }

    /**
     * Get an object.
     * @param id                 Identifier of the object.
     */
    [[nodiscard]] const O& obj(const std::size_t id) const { return *(*objs_)[id]; }

    /**
     * Get the number of objects attached to a node.
     */
    [[nodiscard]] std::size_t nbAssigned() const { return nbAssigned_; }

    /**
     * Get the number of objects not attached to a node.
     */
    [[nodiscard]] std::size_t nbNotAssigned() const { return nbObjs() - nbAssigned_; }

    /**
     * Get the objects not attached (in the order of their identifiers).
     * @param objs               Vector filled with the objects (its previous content is erased).
     */
    void notAssigned(std::vector<const O*>& objs) const
    {
        objs.clear();
        for (std::size_t id = 0; id < objsAss_.size(); ++id)
            if (objsAss_[id] == cNoRef)
                objs.push_back((*objs_)[id]);
    }

    /**
     * Add a new node to the tree.
     * @param parent             Identifier of the parent node (cNoRef for a top node).
     * @param attrs              Attributes of the node.
     * @return the identifier of the node.
     * @exception RGAException   The parent node is not used.
     */
    std::size_t addNode(const std::size_t parent, const RHGAAttrList& attrs)
    {
        mLogModule("r.hga");
        if ((parent != cNoRef) && ((parent >= nodes_.size()) || (!nodes_[parent].reserved_)))
            mThrowRGAException("Node ", parent, " is not used");
        const auto it = std::find_if(nodes_.begin(), nodes_.end(), [](const N& node) { return !node.reserved_; });
        const auto id = static_cast<std::size_t>(it - nodes_.begin());
        if (id == nodes_.size())
            nodes_.push_back(newNode(id));
        auto& node = nodes_[id];
        node.reserved_ = true;
        node.attrs_ = attrs;
        used_.push_back(id);
        attach(parent, id);
        return id;
    }

    /**
     * Delete a node: its child nodes are deleted and its objects are detached.
     * @param id                 Identifier of the node.
     */
    void deleteNode(const std::size_t id)
    {
        auto& node = nodes_[id];
        if (!node.reserved_)
            return;
        while (!node.children_.empty())
            deleteNode(node.children_.back());
        deleteObjs(id);
        detach(id);
        resetNode(node);
        used_.erase(std::find(used_.begin(), used_.end(), id));
    }

    /**
     * Move a node (with its child nodes and its objects) under another parent.
     * @param to                 Identifier of the new parent (cNoRef to make it a top node).
     * @param id                 Identifier of the node.
     * @exception RGAException   The new parent is the node itself or one of its descendants.
     */
    void moveNode(const std::size_t to, const std::size_t id)
    {
        mLogModule("r.hga");
        if (nodes_[id].parent_ == to)
            return;
        for (auto cur = to; cur != cNoRef; cur = nodes_[cur].parent_)
            if (cur == id)
                mThrowRGAException("Node ", id, " cannot be moved under itself");
        detach(id);
        attach(to, id);
    }

    /**
     * Attach an object to a node (the method postInsertObj of the node is called).
     * @param id                 Identifier of the node.
     * @param obj                Object.
     * @exception RGAException   The node is not used or the object is already attached.
     */
    void insertObj(const std::size_t id, const O& obj)
    {
        mLogModule("r.hga");
        auto& node = nodes_[id];
        if (!node.reserved_)
            mThrowRGAException("Node ", id, " is not used");
        if (objsAss_[obj.id()] != cNoRef)
            mThrowRGAException("Object ", obj.id(), " is already attached to node ", objsAss_[obj.id()]);
        node.objs_.push_back(&obj);
        objsAss_[obj.id()] = id;
        ++nbAssigned_;
        node.postInsertObj(obj);
    }

    /**
     * Detach an object from its node (the method postDeleteObj of the node is called).
     * @param obj                Object.
     * @exception RGAException   The object is not attached.
     */
    void deleteObj(const O& obj)
    {
        mLogModule("r.hga");
        const auto id = objsAss_[obj.id()];
        if (id == cNoRef)
            mThrowRGAException("Object ", obj.id(), " is not attached");
        auto& node = nodes_[id];
        node.objs_.erase(std::find(node.objs_.begin(), node.objs_.end(), &obj));
        objsAss_[obj.id()] = cNoRef;
        --nbAssigned_;
        node.postDeleteObj(obj);
    }

    /**
     * Detach all the objects of a node.
     * @param id                 Identifier of the node.
     */
    void deleteObjs(const std::size_t id)
    {
        auto& node = nodes_[id];
        const auto objs = std::move(node.objs_);
        node.objs_.clear();
        for (const auto obj : objs)
        {
            objsAss_[obj->id()] = cNoRef;
            --nbAssigned_;
            node.postDeleteObj(*obj);
        }
    }

    /**
     * Move all the objects of a node to another one.
     * @param to                 Identifier of the destination node.
     * @param from               Identifier of the origin node.
     */
    void moveObjs(const std::size_t to, const std::size_t from)
    {
        const auto objs = nodes_[from].objs_;
        for (const auto obj : objs)
        {
            deleteObj(*obj);
            insertObj(to, *obj);
        }
    }

    /**
     * Get a node.
     * @param id                 Identifier of the node.
     */
    [[nodiscard]] N& node(const std::size_t id) { return nodes_[id]; }

    /**
     * Get a node.
     * @param id                 Identifier of the node.
     */
    [[nodiscard]] const N& node(const std::size_t id) const { return nodes_[id]; }

    /**
     * Get the identifiers of the nodes used (in the order in which they were added).
     */
    [[nodiscard]] const std::vector<std::size_t>& used() const { return used_; }

    /**
     * Get the number of nodes used.
     */
    [[nodiscard]] std::size_t nbNodes() const { return used_.size(); }

    /**
     * Get the identifiers of the top nodes.
     */
    [[nodiscard]] const std::vector<std::size_t>& topNodes() const { return top_; }

    /**
     * Get the identifiers of the child nodes of a node.
     * @param id                 Identifier of the node (cNoRef for the top nodes).
     */
    [[nodiscard]] const std::vector<std::size_t>& children(const std::size_t id) const
    {
        return (id == cNoRef) ? top_ : nodes_[id].children_;
    }

    /**
     * Get the node of an object.
     * @param id                 Identifier of the object.
     * @return cNoRef if the object is not attached.
     */
    [[nodiscard]] std::size_t nodeId(const std::size_t id) const { return objsAss_[id]; }

    /**
     * Get the maximal depth of the tree (1 if there are only top nodes, 0 if the tree is empty).
     */
    [[nodiscard]] std::size_t depth() const
    {
        std::size_t max = 0;
        for (const auto id : top_)
            max = std::max(max, depth(id));
        return max;
    }

    /**
     * Get all the objects attached to a node or to its descendants.
     * @param id                 Identifier of the node.
     * @param objs               Vector where the objects are added.
     */
    void allObjs(const std::size_t id, std::vector<const O*>& objs) const
    {
        const auto& node = nodes_[id];
        objs.insert(objs.end(), node.objs_.begin(), node.objs_.end());
        for (const auto child : node.children_)
            allObjs(child, objs);
    }

    /**
     * Copy another tree into the current one (which is emptied), except a branch and some objects. A node which has no
     * child node and no object once copied is removed.
     * @param from               Tree to copy (it must be a different tree sharing the same objects).
     * @param excluded           Identifier of the node of 'from' which (with its descendants) is not copied (cNoRef to
     *                           copy all the nodes).
     * @param skip               If not null, skip[i] is true if object i must not be copied.
     * @return the identifier of the node which corresponds to the parent of the excluded node (or to its nearest
     * ancestor that was kept), cNoRef if the excluded node is a top node, has no ancestor kept, or is not found.
     * @exception RGAException   The two trees are the same.
     */
    std::size_t copyExceptBranch(const RHGATree& from, const std::size_t excluded = cNoRef, const std::vector<char>* skip = nullptr)
    {
        mLogModule("r.hga");
        if (&from == this)
            mThrowRGAException("A tree cannot be copied into itself");
        clearTree();
        std::size_t ret = cNoRef;
        for (const auto top : from.top_)
        {
            if (top == excluded)
                continue;
            const auto id = addNode(cNoRef, from.nodes_[top].attrs_);
            if (const auto find = copyNode(id, from, top, excluded, skip); find != cNoRef)
                ret = find;
            if (isEmpty(id))
            {
                if (ret == id)
                    ret = cNoRef;
                deleteNode(id);
            }
        }
        return ret;
    }

    /**
     * Copy a branch of another tree under a node of the current tree. A node which has no child node and no object once
     * copied is removed.
     * @param to                 Identifier of the parent of the copy (cNoRef to copy the branch as a top node).
     * @param from               Tree containing the branch (it must be a different tree sharing the same objects).
     * @param branch             Identifier of the root of the branch in 'from'.
     * @param skip               If not null, skip[i] is true if object i must not be copied.
     * @return the identifier of the copy of the root of the branch (cNoRef if it was removed because empty).
     */
    std::size_t copyBranch(const std::size_t to, const RHGATree& from, const std::size_t branch, const std::vector<char>* skip = nullptr)
    {
        const auto id = addNode(to, from.nodes_[branch].attrs_);
        copyNode(id, from, branch, cNoRef, skip);
        if (isEmpty(id))
        {
            deleteNode(id);
            return cNoRef;
        }
        return id;
    }

    /**
     * Remove the nodes that have no child node and no object (until there are no more such nodes).
     * @return the number of nodes removed.
     */
    std::size_t cleanUp()
    {
        std::size_t nb = 0;
        auto again = true;
        while (again)
        {
            again = false;
            tmpIds_ = used_;
            for (const auto id : tmpIds_)
                if (nodes_[id].reserved_ && isEmpty(id))
                {
                    deleteNode(id);
                    ++nb;
                    again = true;
                }
        }
        return nb;
    }

    /**
     * Compute a canonical representation of the tree, which does not depend on the identifiers and on the order of the
     * nodes and of the objects. It is used by sameTree and must be called after each modification of the tree (the
     * genetic operators of RHGAChromo do it).
     */
    void computeCanonical()
    {
        canonical_ = canonical(top_);
    }

    /**
     * Look if two trees represent the same hierarchy (whatever the identifiers and the order of the nodes).
     * computeCanonical must have been called on both.
     * @param tree               Tree to compare with.
     */
    [[nodiscard]] bool sameTree(const RHGATree& tree) const { return canonical_ == tree.canonical_; }

    /**
     * Verify the coherence of the tree:
     * - The links between the nodes and the assignment of the objects.
     * - Each node used has at least one child node or one object.
     * - The attributes of a node strictly include those of its parent.
     * - The objects attached to a node have the same attributes as it.
     *
     * The method verify of each node is also called.
     * @param complete           Must all the objects be attached?
     * @exception RGAException   The tree is incoherent.
     */
    void verifyTree(const bool complete) const
    {
        mLogModule("r.hga");
        std::size_t nb = 0;
        for (const auto id : used_)
        {
            const auto& node = nodes_[id];
            if ((!node.reserved_) || (node.id_ != id))
                mThrowRGAException("Node ", id, " is used but not reserved or has another identifier");
            if (node.parent_ == cNoRef)
            {
                if (std::find(top_.begin(), top_.end(), id) == top_.end())
                    mThrowRGAException("Node ", id, " has no parent but is not a top node");
            }
            else
            {
                const auto& parent = nodes_[node.parent_];
                if ((!parent.reserved_) || (std::find(parent.children_.begin(), parent.children_.end(), id) == parent.children_.end()))
                    mThrowRGAException("Node ", id, " is not a child of its parent ", node.parent_);
                if ((!parent.attrs_.isParentOf(node.attrs_)) || (parent.attrs_.size() >= node.attrs_.size()))
                    mThrowRGAException("The attributes of node ", id, " (", node.attrs_.toString(), ") do not strictly include those of its parent (", parent.attrs_.toString(), ")");
            }
            if (node.children_.empty() && node.objs_.empty())
                mThrowRGAException("Node ", id, " has no child node and no object");
            for (const auto child : node.children_)
                if (nodes_[child].parent_ != id)
                    mThrowRGAException("Node ", child, " is a child of node ", id, " but has another parent");
            for (const auto obj : node.objs_)
            {
                if (objsAss_[obj->id()] != id)
                    mThrowRGAException("Object ", obj->id(), " is in node ", id, " but assigned to ", objsAss_[obj->id()]);
                if (obj->attrs() != node.attrs_)
                    mThrowRGAException("Object ", obj->id(), " has not the attributes of node ", id);
            }
            nb += node.objs_.size();
            node.verify();
        }
        for (const auto id : top_)
            if ((!nodes_[id].reserved_) || (nodes_[id].parent_ != cNoRef))
                mThrowRGAException("Top node ", id, " is not used or has a parent");
        if (nb != nbAssigned_)
            mThrowRGAException(nb, " objects in the nodes but ", nbAssigned_, " attached");
        if (complete && (nbAssigned_ != nbObjs()))
            mThrowRGAException(nbNotAssigned(), " object(s) not attached");
    }

protected:

    /**
     * Create a new (free) node. By default, the node is constructed with its identifier. A derived class overrides this
     * method if the nodes need other parameters.
     * @param id                 Identifier of the node.
     * @exception RGAException   N cannot be constructed from an identifier (the method must then be overridden).
     */
    [[nodiscard]] virtual N newNode(const std::size_t id) const
    {
        if constexpr (std::is_constructible_v<N, std::size_t>)
            return N(id);
        else
        {
            mLogModule("r.hga");
            mThrowRGAException("N is not constructible from an identifier: newNode must be overridden");
        }
    }

private:

    /**
     * Look if a node has no child node and no object.
     * @param id                 Identifier of the node.
     */
    [[nodiscard]] bool isEmpty(const std::size_t id) const { return nodes_[id].children_.empty() && nodes_[id].objs_.empty(); }

    /**
     * Attach a node to a parent.
     * @param parent             Identifier of the parent (cNoRef for the top).
     * @param id                 Identifier of the node.
     */
    void attach(const std::size_t parent, const std::size_t id)
    {
        nodes_[id].parent_ = parent;
        if (parent == cNoRef)
            top_.push_back(id);
        else
        {
            nodes_[parent].children_.push_back(id);
            nodes_[parent].postInsertNode(id);
        }
    }

    /**
     * Detach a node from its parent.
     * @param id                 Identifier of the node.
     */
    void detach(const std::size_t id)
    {
        const auto parent = nodes_[id].parent_;
        if (parent == cNoRef)
            top_.erase(std::find(top_.begin(), top_.end(), id));
        else
        {
            auto& children = nodes_[parent].children_;
            children.erase(std::find(children.begin(), children.end(), id));
            nodes_[parent].postDeleteNode(id);
        }
        nodes_[id].parent_ = cNoRef;
    }

    /**
     * Reset a node that is released.
     * @param node               Node.
     */
    static void resetNode(N& node)
    {
        node.reserved_ = false;
        node.parent_ = cNoRef;
        node.children_.clear();
        node.objs_.clear();
        node.attrs_.clear();
        node.clear();
    }

    /**
     * Copy the content (information, descendants and objects) of a node of another tree into a node (see
     * copyExceptBranch).
     * @param to                 Identifier of the destination node.
     * @param from               Tree used as source.
     * @param fromId             Identifier of the node to copy.
     * @param excluded           Identifier of the node not copied.
     * @param skip               Objects not copied.
     * @return the identifier of the node which corresponds to the parent of the excluded node (cNoRef if not found).
     */
    std::size_t copyNode(const std::size_t to, const RHGATree& from, const std::size_t fromId, const std::size_t excluded, const std::vector<char>* skip)
    {
        const auto& src = from.nodes_[fromId];
        nodes_[to].copyInfos(src);
        std::size_t ret = cNoRef;
        for (const auto child : src.children_)
        {
            if (child == excluded)
            {
                ret = to;
                continue;
            }
            const auto id = addNode(to, from.nodes_[child].attrs_);
            if (const auto find = copyNode(id, from, child, excluded, skip); find != cNoRef)
                ret = find;
            if (isEmpty(id))
            {
                if (ret == id)
                    ret = to;
                deleteNode(id);
            }
        }
        for (const auto obj : src.objs_)
            if ((skip == nullptr) || (!(*skip)[obj->id()]))
                insertObj(to, *obj);
        return ret;
    }

    /**
     * Get the depth of the branch starting from a node.
     * @param id                 Identifier of the node.
     */
    [[nodiscard]] std::size_t depth(const std::size_t id) const
    {
        std::size_t max = 0;
        for (const auto child : nodes_[id].children_)
            max = std::max(max, depth(child));
        return max + 1;
    }

    /**
     * Compute the canonical representation of a set of sibling nodes.
     * @param ids                Identifiers of the nodes.
     */
    [[nodiscard]] std::string canonical(const std::vector<std::size_t>& ids) const
    {
        std::vector<std::string> codes;
        codes.reserve(ids.size());
        for (const auto id : ids)
        {
            const auto& node = nodes_[id];
            std::vector<std::size_t> objs;
            objs.reserve(node.objs_.size());
            for (const auto obj : node.objs_)
                objs.push_back(obj->id());
            std::sort(objs.begin(), objs.end());
            std::string code = "(" + node.attrs_.toString() + "|";
            for (const auto obj : objs)
                code += std::to_string(obj) + ",";
            code += "|" + canonical(node.children_) + ")";
            codes.push_back(std::move(code));
        }
        std::sort(codes.begin(), codes.end());
        std::string res;
        for (const auto& code : codes)
            res += code;
        return res;
    }

    //-----------------------------------------------------
    const std::vector<const O*>* objs_ = nullptr;   ///< Objects to place (shared).
    std::deque<N> nodes_;                   ///< All the nodes (the identifier is the index).
    std::vector<std::size_t> used_;         ///< Identifiers of the nodes used.
    std::vector<std::size_t> top_;          ///< Identifiers of the top nodes.
    std::vector<std::size_t> objsAss_;      ///< Node of each object (cNoRef if not attached).
    std::size_t nbAssigned_ = 0;            ///< Number of objects attached.
    std::string canonical_;                 ///< Canonical representation (see computeCanonical).
    std::vector<std::size_t> tmpIds_;       ///< Temporary identifiers.
};


}  //------- End of namespace evocomb ----------------------------------------------
