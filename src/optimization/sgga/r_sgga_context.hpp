/*

    EvoComb

    RSGGAContext.hpp

    Context of the Similarity-based Grouping Genetic Algorithm - Header.

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
#include <utility>
#include <vector>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "app/r_logger.hpp"
#include "ga/r_ga.hpp"
#include "gga/r_gga_context.hpp"
#include "matrix/r_max_vector.hpp"
#include "sgga/r_sgga_data.hpp"
#include "tools/r_json.hpp"
#include "tools/r_std.hpp"


//------------------------------------------------------------------------------
namespace evocomb {
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
 * The RSGGAContext class provides the context of the Similarity-based Grouping Genetic Algorithm: the objects, their
 * data (see RSGGAData) and the parameters. The parameters are read from the "sgga" object of the configuration (all of
 * them are optional):
 * @code
 * {
 *   "sgga": {
 *     "min_sim_level": 0.0,        // Minimal similarity between two objects of a group.
 *     "all_min_sim": false,        // Must all the pairs of a group respect min_sim_level (or only one per object)?
 *     "min_agreement": 1.0,        // Objects with a higher agreement ratio should be grouped together.
 *     "min_disagreement": 1.0,     // Objects with a higher disagreement ratio cannot be grouped together.
 *     "nb_min_objs": 0,            // Minimal number of objects per group.
 *     "nb_max_objs": 0,            // Maximal number of objects per group (0 = no limit).
 *     "max_kmeans": 10,            // Maximal number of iterations of a k-Means.
 *     "convergence": 0.0001,       // A k-Means stops when the ratio of modified centroids is lower or equal.
 *     "nb_div_chromos": 2,         // Number of divided (and merged) solutions built by the optimization (0 = no optimization).
 *     "criteria": [ ... ]          // PROMETHEE criteria used by the optimization (see below).
 *   }
 * }
 * @endcode
 * The optimization of a chromosome (see RSGGAChromo::optimize) compares several solutions with PROMETHEE II on the
 * three criteria of the SGGA (similarity, agreement and disagreement). The criteria are, by order of priority, those of
 * "sgga/criteria", those of the PROMETHEE fitness of the GA ("ga/fitness/criteria") or default ones (see
 * RPromKernel::addCriteria).
 *
 * The default heuristic is "sg_first_fit" (see RSGGAHeuristic).
 * @tparam O                     Class of the objects.
 * @short SGGA Context.
 */
template<class O>
    class RSGGAContext : public RGGAContext<O>
{
public:

    /**
     * Construct the context.
     * @param objs               Objects to group (objs[i]->id() must be i).
     * @param data               Data of the objects (it must live as long as the context).
     */
    RSGGAContext(std::vector<const O*> objs, const RSGGAData& data) :
        RGGAContext<O>(std::move(objs)),
        data_(&data),
        criteria_(std::make_shared<const RJSON>(RJSON::parse(R"([
            {"name": "similarity", "type": "maximize"},
            {"name": "agreement", "type": "maximize"},
            {"name": "disagreement", "type": "minimize"}])")))
    {
        this->heuristic("sg_first_fit");
    }

    /**
     * Read the parameters from the "gga" and "sgga" objects of a configuration, and build the lists of close objects
     * (see closeObjs).
     * @param config             Configuration.
     * @exception RGAException   A parameter is invalid, or the data do not correspond to the objects.
     */
    void configure(const RJSON& config) override
    {
        mLogModule("r.sgga");
        RGGAContext<O>::configure(config);
        if (data_->nbObjs() != this->objs().size())
            mThrowRGAException("The data concern ", data_->nbObjs(), " objects instead of ", this->objs().size());

        // Criteria of the PROMETHEE fitness of the GA (if any)
        if (const auto fitness = config.find(std::vector<std::string>{"ga", "fitness"}); (fitness != nullptr) && fitness->isObject())
        {
            const auto type = fitness->find("type");
            const auto criteria = fitness->find("criteria");
            if ((type != nullptr) && type->isString() && (type->toString() == "promethee") && (criteria != nullptr))
                criteria_ = std::make_shared<const RJSON>(*criteria);
        }

        // Parameters of the SGGA
        if (const auto sgga = config.find("sgga"); sgga != nullptr)
        {
            if (!sgga->isObject())
                mThrowRGAException("'sgga' must be a JSON object");
            this->readDouble(*sgga, "min_sim_level", minSimLevel_);
            this->readBool(*sgga, "all_min_sim", allMinSim_);
            this->readDouble(*sgga, "min_agreement", minAgreement_);
            this->readDouble(*sgga, "min_disagreement", minDisagreement_);
            this->readSize(*sgga, "nb_min_objs", nbMinObjs_);
            this->readSize(*sgga, "nb_max_objs", nbMaxObjs_);
            this->readSize(*sgga, "max_kmeans", maxKMeans_);
            this->readDouble(*sgga, "convergence", convergence_);
            this->readSize(*sgga, "nb_div_chromos", nbDivChromos_);
            if (const auto criteria = sgga->find("criteria"); criteria != nullptr)
                criteria_ = std::make_shared<const RJSON>(*criteria);
        }
        if (!criteria_->isArray())
            mThrowRGAException("The PROMETHEE criteria must be a JSON array");

        buildCloseObjs();
    }

    /**
     * Get the data of the objects.
     */
    [[nodiscard]] const RSGGAData& data() const { return *data_; }

    /**
     * Get the minimal similarity between two objects of a group.
     */
    [[nodiscard]] double minSimLevel() const { return minSimLevel_; }

    /**
     * Look if all the pairs of objects of a group must have a similarity greater or equal to the minimal similarity
     * (`true`), or if each object must only have such a similarity with at least one other object of its group
     * (`false`).
     */
    [[nodiscard]] bool allMinSim() const { return allMinSim_; }

    /**
     * Get the minimal agreement ratio: an object is preferably grouped with the objects with which it has a greater or
     * equal agreement ratio.
     */
    [[nodiscard]] double minAgreement() const { return minAgreement_; }

    /**
     * Get the minimal disagreement ratio: two objects whose disagreement ratio is greater or equal cannot be grouped
     * together.
     */
    [[nodiscard]] double minDisagreement() const { return minDisagreement_; }

    /**
     * Get the minimal number of objects per group.
     */
    [[nodiscard]] std::size_t nbMinObjs() const { return nbMinObjs_; }

    /**
     * Get the maximal number of objects per group (0 means no limit).
     */
    [[nodiscard]] std::size_t nbMaxObjs() const { return nbMaxObjs_; }

    /**
     * Get the maximal number of iterations of a k-Means.
     */
    [[nodiscard]] std::size_t maxKMeans() const { return maxKMeans_; }

    /**
     * Get the convergence threshold of a k-Means: it stops when the ratio of modified centroids between two iterations
     * is lower or equal.
     */
    [[nodiscard]] double convergence() const { return convergence_; }

    /**
     * Get the number of divided (and of merged) solutions built by the optimization of a chromosome (0 means no
     * optimization).
     */
    [[nodiscard]] std::size_t nbDivChromos() const { return nbDivChromos_; }

    /**
     * Get the PROMETHEE criteria used by the optimization.
     */
    [[nodiscard]] const RJSON& criteria() const { return *criteria_; }

    /**
     * Get the objects "close" to a given one, i.e. with which it has an agreement ratio greater or equal to the minimal
     * agreement ratio (ordered by decreasing agreement ratios).
     * @param id                 Identifier of the object.
     */
    [[nodiscard]] const RMaxVector& closeObjs(const std::size_t id) const { return closeObjs_[id]; }

    /**
     * Look if two objects can be in the same group: they have not the same parent, their disagreement ratio is lower
     * than the minimal one and, if allMinSim is true, their similarity is greater or equal to the minimal one.
     * @param obj1               First object.
     * @param obj2               Second object.
     */
    [[nodiscard]] bool compatible(const O& obj1, const O& obj2) const
    {
        const auto id1 = obj1.id();
        const auto id2 = obj2.id();
        if (const auto parent = data_->parent(id1); (parent != cNoRef) && (parent == data_->parent(id2)))
            return false;
        if (data_->disagreement(id1, id2) >= minDisagreement_)
            return false;
        if (allMinSim_ && (data_->sim(id1, id2) < minSimLevel_))
            return false;
        return true;
    }

private:

    /**
     * Build the lists of close objects (see closeObjs).
     */
    void buildCloseObjs()
    {
        const auto nb = this->objs().size();
        closeObjs_.assign(nb, RMaxVector());
        for (std::size_t i = 0; i < nb; ++i)
            for (std::size_t j = i + 1; j < nb; ++j)
                if (const auto ratio = data_->agreement(i, j); ratio >= minAgreement_)
                {
                    closeObjs_[i].add(j, ratio);
                    closeObjs_[j].add(i, ratio);
                }
    }

    //-----------------------------------------------------
    const RSGGAData* data_;                 ///< Data of the objects.
    double minSimLevel_ = 0.0;              ///< Minimal similarity.
    bool allMinSim_ = false;                ///< All the pairs must respect the minimal similarity?
    double minAgreement_ = 1.0;             ///< Minimal agreement ratio.
    double minDisagreement_ = 1.0;          ///< Minimal disagreement ratio.
    std::size_t nbMinObjs_ = 0;             ///< Minimal number of objects per group.
    std::size_t nbMaxObjs_ = 0;             ///< Maximal number of objects per group (0 = no limit).
    std::size_t maxKMeans_ = 10;            ///< Maximal number of k-Means iterations.
    double convergence_ = 0.0001;           ///< Convergence threshold of the k-Means.
    std::size_t nbDivChromos_ = 2;          ///< Number of divided solutions of the optimization.
    std::shared_ptr<const RJSON> criteria_; ///< PROMETHEE criteria of the optimization.
    std::vector<RMaxVector> closeObjs_;     ///< Close objects of each object.
};


}  //------- End of namespace evocomb ----------------------------------------------
