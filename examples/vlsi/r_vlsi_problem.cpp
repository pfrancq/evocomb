/*

    EvoComb - VLSI example

    RVLSIProblem.cpp

    VLSI Placement Problem - Implementation.

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
// include files for ANSI C/C++
#include <map>
#include <utility>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "app/r_logger.hpp"
#include "r_vlsi_problem.hpp"
#include "tools/r_exception.hpp"
using namespace evocomb;
using namespace evocomb::vlsi;


//------------------------------------------------------------------------------
mLogModule("r.vlsi");


//------------------------------------------------------------------------------
namespace {

/**
 * Get a mandatory element of an object.
 * @param obj                    JSON object.
 * @param key                    Key.
 * @param where                  Description of the object (for the errors).
 */
const RJSON& get(const RJSON& obj, const std::string& key, const std::string& where)
{
    const auto param = obj.find(key);
    if (param == nullptr)
        mThrowRException(where, ": '", key, "' is missing");
    return *param;
}

/**
 * Get a string.
 * @param json                   JSON element.
 * @param where                  Description (for the errors).
 */
std::string string(const RJSON& json, const std::string& where)
{
    if (!json.isString())
        mThrowRException(where, " must be a string");
    return json.toString();
}

/**
 * Get an integer coordinate.
 * @param json                   JSON element.
 * @param where                  Description (for the errors).
 */
RCoord coord(const RJSON& json, const std::string& where)
{
    if (!json.isInteger())
        mThrowRException(where, " must contain integers");
    return static_cast<RCoord>(json.toInteger());
}

/**
 * Get an array.
 * @param json                   JSON element.
 * @param size                   Expected size (0 for any size).
 * @param where                  Description (for the errors).
 */
const RJSON& array(const RJSON& json, const std::size_t size, const std::string& where)
{
    if ((!json.isArray()) || (size && (json.size() != size)))
    {
        if (size)
            mThrowRException(where, " must be an array of ", size, " elements");
        mThrowRException(where, " must be an array");
    }
    return json;
}

/**
 * Read the pins of the terminals of an object or of the circuit.
 * @param terminals              JSON array of terminals.
 * @param where                  Description (for the errors).
 * @param add                    Function (name, pins) adding a terminal.
 */
template<class F>
    void readTerminals(const RJSON& terminals, const std::string& where, F add)
{
    array(terminals, 0, where + ": 'terminals'");
    for (std::size_t i = 0; i < terminals.size(); ++i)
    {
        const auto& term = terminals[i];
        const auto name = string(get(term, "id", where + ": terminal " + std::to_string(i)), where + ": 'id'");
        const auto termWhere = where + ": terminal '" + name + "'";
        std::vector<RGA2DPin> pins;
        if (const auto list = term.find("pins"); list != nullptr)
        {
            array(*list, 0, termWhere + ": 'pins'");
            for (std::size_t j = 0; j < list->size(); ++j)
            {
                const auto& p = (*list)[j];
                RGA2DPin pin;
                if (const auto id = p.find("id"); id != nullptr)
                    pin.name = string(*id, termWhere + ": pin 'id'");
                if (const auto layer = p.find("layer"); layer != nullptr)
                    pin.layer = string(*layer, termWhere + ": pin 'layer'");
                const auto& r = array(get(p, "rect", termWhere), 4, termWhere + ": 'rect'");
                pin.rect = RRect(coord(r[0], termWhere), coord(r[1], termWhere), coord(r[2], termWhere), coord(r[3], termWhere));
                pins.push_back(std::move(pin));
            }
        }
        add(name, std::move(pins));
    }
}

/**
 * A master (read once, copied for each of its instances).
 */
struct Master
{
    RPolygon polygon;                               ///< Shape.
    std::vector<std::pair<std::string, std::vector<RGA2DPin>>> terminals;   ///< Terminals.
};

}



//------------------------------------------------------------------------------
//
// class RVLSIProblem
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
void RVLSIProblem::read(const RJSON& json)
{
    problem_ = std::make_unique<RGA2DProblem>();
    masters_.clear();
    if (!json.isObject())
        mThrowRException("The problem must be a JSON object");

    // Size of the circuit
    const auto& size = array(get(json, "size", "Problem"), 2, "'size'");
    problem_->limits(coord(size[0], "'size'"), coord(size[1], "'size'"));

    // Terminals of the circuit
    if (const auto terminals = json.find("terminals"); terminals != nullptr)
        readTerminals(*terminals, "Circuit", [this](const std::string& name, std::vector<RGA2DPin> pins) {
            if (problem_->findBoardConnector(name) != cNoRef)
                mThrowRException("Circuit: terminal '", name, "' is defined twice");
            auto& con = problem_->addBoardConnector(name);
            for (auto& pin : pins)
                con.addPin(std::move(pin));
        });

    // Masters
    std::map<std::string, Master> masters;
    const auto& list = array(get(json, "masters", "Problem"), 0, "'masters'");
    for (std::size_t i = 0; i < list.size(); ++i)
    {
        const auto& m = list[i];
        const auto name = string(get(m, "id", "Master " + std::to_string(i)), "Master 'id'");
        const auto where = "Master '" + name + "'";
        if (masters.count(name))
            mThrowRException(where, " is defined twice");
        auto& master = masters[name];
        const auto& poly = array(get(m, "polygon", where), 0, where + ": 'polygon'");
        std::vector<RPoint> vertices;
        for (std::size_t j = 0; j < poly.size(); ++j)
        {
            const auto& pt = array(poly[j], 2, where + ": vertex");
            vertices.emplace_back(coord(pt[0], where), coord(pt[1], where));
        }
        master.polygon = RPolygon(std::move(vertices));
        if (!master.polygon.isRectilinear())
            mThrowRException(where, ": the polygon is not rectilinear");
        if (const auto terminals = m.find("terminals"); terminals != nullptr)
            readTerminals(*terminals, where, [&master](const std::string& term, std::vector<RGA2DPin> pins) {
                master.terminals.emplace_back(term, std::move(pins));
            });
    }

    // Instances
    const auto& instances = array(get(json, "instances", "Problem"), 0, "'instances'");
    for (std::size_t i = 0; i < instances.size(); ++i)
    {
        const auto& inst = instances[i];
        const auto name = string(get(inst, "id", "Instance " + std::to_string(i)), "Instance 'id'");
        const auto where = "Instance '" + name + "'";
        if (problem_->findObj(name) != cNoRef)
            mThrowRException(where, " is defined twice");
        const auto masterName = string(get(inst, "master", where), where + ": 'master'");
        const auto it = masters.find(masterName);
        if (it == masters.end())
            mThrowRException(where, ": master '", masterName, "' does not exist");
        auto& obj = problem_->addObj(name, it->second.polygon);
        for (const auto& [term, pins] : it->second.terminals)
        {
            const auto con = obj.connectors().size();
            obj.addConnector(term);
            for (const auto& pin : pins)
                obj.addPin(con, pin);
        }
        masters_.push_back(masterName);
    }

    // Nets
    if (const auto nets = json.find("nets"); nets != nullptr)
    {
        array(*nets, 0, "'nets'");
        for (std::size_t i = 0; i < nets->size(); ++i)
        {
            const auto& n = (*nets)[i];
            RGA2DNet net;
            net.name = string(get(n, "id", "Net " + std::to_string(i)), "Net 'id'");
            const auto where = "Net '" + net.name + "'";
            if (const auto weight = n.find("weight"); weight != nullptr)
            {
                if (weight->isInteger())
                    net.weight = static_cast<double>(weight->toInteger());
                else if (weight->isDecimal())
                    net.weight = weight->toDecimal();
                else
                    mThrowRException(where, ": 'weight' must be a number");
            }
            const auto& connect = array(get(n, "connect", where), 0, where + ": 'connect'");
            for (std::size_t j = 0; j < connect.size(); ++j)
            {
                const auto& c = connect[j];
                const auto term = string(get(c, "terminal", where), where + ": 'terminal'");
                RGA2DNet::Item item{cNoRef, cNoRef};
                if (const auto inst = c.find("instance"); inst != nullptr)
                {
                    const auto instName = string(*inst, where + ": 'instance'");
                    item.obj = problem_->findObj(instName);
                    if (item.obj == cNoRef)
                        mThrowRException(where, ": instance '", instName, "' does not exist");
                    item.connector = problem_->obj(item.obj).findConnector(term);
                    if (item.connector == cNoRef)
                        mThrowRException(where, ": instance '", instName, "' has no terminal '", term, "'");
                }
                else
                {
                    item.connector = problem_->findBoardConnector(term);
                    if (item.connector == cNoRef)
                        mThrowRException(where, ": the circuit has no terminal '", term, "'");
                }
                net.items.push_back(item);
            }
            problem_->addNet(std::move(net));
        }
    }

    problem_->init();
}


//------------------------------------------------------------------------------
std::unique_ptr<RJSON> RVLSIProblem::toJSON(const RGA2DLayout& layout) const
{
    const auto pair = [](const RCoord a, const RCoord b) {
        auto res = std::make_unique<RJSON>(RJSONType::Array);
        res->add(std::make_unique<RJSON>(static_cast<long long>(a)));
        res->add(std::make_unique<RJSON>(static_cast<long long>(b)));
        return res;
    };
    auto res = std::make_unique<RJSON>(RJSONType::Array);
    for (std::size_t id = 0; id < problem_->nbObjs(); ++id)
    {
        auto inst = std::make_unique<RJSON>(RJSONType::Object);
        inst->add("id", std::make_unique<RJSON>(problem_->obj(id).name()));
        inst->add("master", std::make_unique<RJSON>(masters_[id]));
        if (layout.isPlaced(id))
        {
            const auto& cfg = layout.config(id);
            const auto& pos = layout.placement(id).pos;
            inst->add("orientation", std::make_unique<RJSON>(to_string(cfg.orientation)));
            inst->add("position", pair(pos.x, pos.y));
            inst->add("size", pair(cfg.width, cfg.height));
        }
        res->add(std::move(inst));
    }
    return res;
}
