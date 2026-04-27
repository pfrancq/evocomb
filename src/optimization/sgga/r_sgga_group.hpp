/*

    EvoComb

    RSGGAGroup.hpp

    Group of the Similarity-based Grouping Genetic Algorithm - Header.

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
#include <limits>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "gga/r_gga_group.hpp"
#include "sgga/r_sgga_context.hpp"


//------------------------------------------------------------------------------
namespace evocomb {
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
 * The RSGGAGroup class provides a group of the Similarity-based Grouping Genetic Algorithm. An object can be inserted
 * in the group if:
 * - The group does not contain the maximal number of objects.
 * - It is compatible with each object of the group (see
 *   RSGGAContext::compatible): no common parent, no disagreement ratio
 *   greater or equal to the minimal disagreement, and (if required) a
 *   minimal similarity.
 * - It has a similarity greater or equal to the minimal similarity with at
 *   least one object of the group.
 *
 * The group maintains (lazily) its centroid, i.e. the object which is the most similar to the other ones, and some
 * averages used to evaluate the solutions. These values are computed when needed and cached until the group is
 * modified.
 * @tparam O                     Class of the objects.
 * @short SGGA Group.
 */
template<class O>
    class RSGGAGroup : public RGGAGroup<O>
{
public:

    /**
     * Construct the group.
     * @param id                 Identifier of the group.
     * @param context            Context (it must live as long as the group).
     */
    RSGGAGroup(const std::size_t id, const RSGGAContext<O>& context) :
        RGGAGroup<O>(id),
        context_(&context)
    {
    }

    /**
     * Get the context.
     */
    [[nodiscard]] const RSGGAContext<O>& context() const { return *context_; }

    /**
     * Look if an object can be inserted in the group (see the class description).
     * @param obj                Object.
     */
    [[nodiscard]] bool canInsert(const O& obj) const override
    {
        double maxSim;
        return canInsert(obj, maxSim);
    }

    /**
     * Look if an object can be inserted in the group (see the class description), and compute its maximal similarity
     * with the objects of the group.
     * @param obj                Object.
     * @param maxSim             Maximal similarity with an object of the group (-infinity if the group is empty or if
     *                           the object cannot be inserted).
     */
    [[nodiscard]] bool canInsert(const O& obj, double& maxSim) const
    {
        maxSim = -std::numeric_limits<double>::infinity();

        // An empty group accepts any object
        if (this->nbObjs() == 0)
            return true;

        // Maximal number of objects
        if (context_->nbMaxObjs() && (this->nbObjs() >= context_->nbMaxObjs()))
            return false;

        // Each object of the group must be compatible, and at least one must have the minimal similarity
        const auto& data = context_->data();
        auto oneMinSim = false;
        double max = -std::numeric_limits<double>::infinity();
        for (const auto member : this->objs())
        {
            if (!context_->compatible(obj, *member))
                return false;
            const auto sim = data.sim(obj.id(), member->id());
            if (sim >= context_->minSimLevel())
                oneMinSim = true;
            if (sim > max)
                max = sim;
        }
        if (!oneMinSim)
            return false;
        maxSim = max;
        return true;
    }

    /**
     * Get the centroid of the group: the object with the highest sum of similarities with the other objects of the
     * group (unless it was fixed by lockCentroid).
     * @return nullptr if the group is empty.
     */
    [[nodiscard]] const O* centroid() const
    {
        computeCentroid();
        return centroid_;
    }

    /**
     * Get the average similarity between the centroid and the other objects of the group (1 if the group contains a
     * single object, 0 if it is empty).
     */
    [[nodiscard]] double avgIntraSim() const
    {
        computeCentroid();
        return avgIntraSim_;
    }

    /**
     * Get the similarity between an object and the centroid of the group.
     * @param obj                Object (the group must not be empty).
     */
    [[nodiscard]] double relSim(const O& obj) const { return context_->data().sim(centroid()->id(), obj.id()); }

    /**
     * Get the agreement measure of the group: the sum of the agreement ratios of the pairs of objects divided by the
     * number of objects (0 if it contains less than two objects). Dividing by the number of objects rather than by the
     * number of pairs favors the large groups whose objects agree.
     */
    [[nodiscard]] double avgAgreement() const
    {
        computeMeasures();
        return avgAgreement_;
    }

    /**
     * Get the disagreement measure of the group: the sum of the disagreement ratios of the pairs of objects divided by
     * the number of objects (0 if it contains less than two objects).
     */
    [[nodiscard]] double avgDisagreement() const
    {
        computeMeasures();
        return avgDisagreement_;
    }

    /**
     * Fix the centroid of the group (for example the prototype of a k-Means):
     * it is not recomputed when the group is modified, until unlockCentroid is called.
     * @param obj                Object of the group.
     */
    void lockCentroid(const O& obj)
    {
        centroid_ = &obj;
        centroidLocked_ = true;
        centroidComputed_ = false;
    }

    /**
     * Release the centroid fixed by lockCentroid (it will be recomputed).
     */
    void unlockCentroid()
    {
        centroidLocked_ = false;
        centroidComputed_ = false;
        centroid_ = nullptr;
    }

    /**
     * Method called after an object was inserted in the group.
     * @param obj                Object.
     */
    void postInsert([[maybe_unused]] const O& obj) override { invalidate(); }

    /**
     * Method called after an object was deleted from the group.
     * @param obj                Object.
     */
    void postDelete([[maybe_unused]] const O& obj) override { invalidate(); }

    /**
     * Method called before all the objects of a group are inserted into the current one: the cached values of the
     * source are copied.
     * @param from               Group used as source.
     * @return `false` since the cached values are already correct.
     */
    bool preInsertAll(const RGGAGroup<O>& from) override
    {
        const auto src = dynamic_cast<const RSGGAGroup*>(&from);
        if (src == nullptr)
            return true;
        if (src->centroidLocked_)
        {
            invalidate();
            centroidLocked_ = false;
            centroid_ = nullptr;
            return false;
        }
        centroid_ = src->centroid_;
        centroidLocked_ = false;
        centroidComputed_ = src->centroidComputed_;
        avgIntraSim_ = src->avgIntraSim_;
        measuresComputed_ = src->measuresComputed_;
        avgAgreement_ = src->avgAgreement_;
        avgDisagreement_ = src->avgDisagreement_;
        return false;
    }

    /**
     * Method called when the group is released.
     */
    void clear() override
    {
        centroidLocked_ = false;
        centroid_ = nullptr;
        invalidate();
    }

private:

    /**
     * Invalidate the cached values (the centroid is kept if it is locked).
     */
    void invalidate()
    {
        centroidComputed_ = false;
        measuresComputed_ = false;
        if (!centroidLocked_)
            centroid_ = nullptr;
    }

    /**
     * Compute the sum of the similarities of an object of the group with the other ones.
     * @param obj                Object.
     */
    [[nodiscard]] double sumSim(const O& obj) const
    {
        const auto& data = context_->data();
        double sum = 0.0;
        for (const auto member : this->objs())
            if (member != &obj)
                sum += data.sim(obj.id(), member->id());
        return sum;
    }

    /**
     * Compute the centroid and the average intra-similarity (if necessary).
     */
    void computeCentroid() const
    {
        if (centroidComputed_)
            return;
        centroidComputed_ = true;
        const auto nb = this->nbObjs();
        if (nb == 0)
        {
            centroid_ = nullptr;
            avgIntraSim_ = 0.0;
            return;
        }
        if (!centroidLocked_)
        {
            // The object with the highest sum of similarities
            centroid_ = this->objs().front();
            auto best = sumSim(*centroid_);
            for (const auto obj : this->objs())
                if (const auto sum = sumSim(*obj); sum > best)
                {
                    best = sum;
                    centroid_ = obj;
                }
        }
        avgIntraSim_ = (nb == 1) ? 1.0 : sumSim(*centroid_) / static_cast<double>(nb - 1);
    }

    /**
     * Compute the agreement and disagreement measures (if necessary).
     */
    void computeMeasures() const
    {
        if (measuresComputed_)
            return;
        measuresComputed_ = true;
        avgAgreement_ = avgDisagreement_ = 0.0;
        const auto& objs = this->objs();
        if (objs.size() < 2)
            return;
        const auto& data = context_->data();
        for (std::size_t i = 0; i < objs.size(); ++i)
            for (std::size_t j = i + 1; j < objs.size(); ++j)
            {
                avgAgreement_ += data.agreement(objs[i]->id(), objs[j]->id());
                avgDisagreement_ += data.disagreement(objs[i]->id(), objs[j]->id());
            }
        avgAgreement_ /= static_cast<double>(objs.size());
        avgDisagreement_ /= static_cast<double>(objs.size());
    }

    //-----------------------------------------------------
    const RSGGAContext<O>* context_;        ///< Context.
    mutable const O* centroid_ = nullptr;   ///< Centroid (if computed or locked).
    bool centroidLocked_ = false;           ///< Is the centroid locked?
    mutable bool centroidComputed_ = false; ///< Are the centroid and avgIntraSim_ computed?
    mutable double avgIntraSim_ = 0.0;      ///< Average similarity with the centroid.
    mutable bool measuresComputed_ = false; ///< Are the agreement measures computed?
    mutable double avgAgreement_ = 0.0;     ///< Agreement measure.
    mutable double avgDisagreement_ = 0.0;  ///< Disagreement measure.
};


}  //------- End of namespace evocomb ----------------------------------------------
