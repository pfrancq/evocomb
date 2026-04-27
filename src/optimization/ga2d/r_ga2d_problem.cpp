/*

    EvoComb

    RGA2DProblem.cpp

    2D Placement Problem - Implementation.

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
#include <algorithm>
#include <utility>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "app/r_logger.hpp"
#include "ga2d/r_ga2d_problem.hpp"
#include "tools/r_exception.hpp"
using namespace evocomb;


//------------------------------------------------------------------------------
mLogModule("r.2dga");



//------------------------------------------------------------------------------
//
// class RGA2DConnector
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
RGA2DConnector::RGA2DConnector(const std::size_t obj, const std::size_t id, std::string name) :
    obj_(obj),
    id_(id),
    name_(std::move(name))
{
}



//------------------------------------------------------------------------------
//
// class RGA2DObj
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
RGA2DObj::RGA2DObj(const std::size_t id, std::string name, const RPolygon& polygon) :
    id_(id),
    name_(std::move(name))
{
    const auto bound = polygon.boundary();
    translation_ = RPoint(-bound.x1, -bound.y1);
    polygon_ = polygon.translated(translation_);
}


//------------------------------------------------------------------------------
RGA2DConnector& RGA2DObj::addConnector(const std::string& name)
{
    connectors_.emplace_back(id_, connectors_.size(), name);
    return connectors_.back();
}


//------------------------------------------------------------------------------
void RGA2DObj::addPin(const std::size_t connector, RGA2DPin pin)
{
    if (connector >= connectors_.size())
        mThrowRException("Object '", name_, "' has no connector ", connector);
    pin.rect = pin.rect.translated(translation_);
    connectors_[connector].addPin(std::move(pin));
}


//------------------------------------------------------------------------------
std::size_t RGA2DObj::findConnector(const std::string& name) const
{
    const auto it = std::find_if(connectors_.begin(), connectors_.end(), [&name](const RGA2DConnector& con) { return con.name() == name; });
    return (it == connectors_.end()) ? cNoRef : static_cast<std::size_t>(it - connectors_.begin());
}


//------------------------------------------------------------------------------
void RGA2DObj::initConfigs()
{
    if (!polygon_.isRectilinear())
        mThrowRException("The shape of object '", name_, "' is not rectilinear");
    configs_.clear();
    for (const auto ori : cOrientations)
    {
        RGA2DConfig config;
        RPoint offset;
        config.orientation = ori;
        config.polygon = polygon_.transformed(ori, offset);
        config.rects = config.polygon.rects();
        const auto bound = config.polygon.boundary();
        config.width = bound.width();
        config.height = bound.height();
        config.area = config.polygon.area();
        for (const auto& con : connectors_)
        {
            std::vector<RRect> rects;
            std::vector<RPoint> centers;
            for (const auto& pin : con.pins())
            {
                rects.push_back(RPolygon::transform(pin.rect, ori).translated(offset));
                centers.push_back(rects.back().center());
            }
            config.pins.push_back(std::move(rects));
            config.centers.push_back(std::move(centers));
        }

        // Keep the configuration only if it is different from the previous ones (same shape and same pins)
        const auto same = std::any_of(configs_.begin(), configs_.end(), [&config](const RGA2DConfig& cfg) {
            return (cfg.rects == config.rects) && (cfg.pins == config.pins);
        });
        if (!same)
            configs_.push_back(std::move(config));
    }
}



//------------------------------------------------------------------------------
//
// class RGA2DProblem
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
void RGA2DProblem::limits(const RCoord width, const RCoord height)
{
    if ((width <= 0) || (height <= 0))
        mThrowRException("Invalid limits ", width, "x", height);
    width_ = width;
    height_ = height;
}


//------------------------------------------------------------------------------
RGA2DObj& RGA2DProblem::addObj(const std::string& name, const RPolygon& polygon)
{
    objs_.push_back(std::make_unique<RGA2DObj>(objs_.size(), name, polygon));
    return *objs_.back();
}


//------------------------------------------------------------------------------
std::size_t RGA2DProblem::findObj(const std::string& name) const
{
    const auto it = std::find_if(objs_.begin(), objs_.end(), [&name](const auto& obj) { return obj->name() == name; });
    return (it == objs_.end()) ? cNoRef : static_cast<std::size_t>(it - objs_.begin());
}


//------------------------------------------------------------------------------
RGA2DConnector& RGA2DProblem::addBoardConnector(const std::string& name)
{
    board_.emplace_back(cNoRef, board_.size(), name);
    return board_.back();
}


//------------------------------------------------------------------------------
std::size_t RGA2DProblem::findBoardConnector(const std::string& name) const
{
    const auto it = std::find_if(board_.begin(), board_.end(), [&name](const RGA2DConnector& con) { return con.name() == name; });
    return (it == board_.end()) ? cNoRef : static_cast<std::size_t>(it - board_.begin());
}


//------------------------------------------------------------------------------
void RGA2DProblem::addNet(RGA2DNet net)
{
    for (const auto& item : net.items)
    {
        if (item.obj == cNoRef)
        {
            if (item.connector >= board_.size())
                mThrowRException("Net '", net.name, "': the board has no connector ", item.connector);
        }
        else
        {
            if (item.obj >= objs_.size())
                mThrowRException("Net '", net.name, "': object ", item.obj, " does not exist");
            if (item.connector >= objs_[item.obj]->connectors().size())
                mThrowRException("Net '", net.name, "': object '", objs_[item.obj]->name(), "' has no connector ", item.connector);
        }
    }
    nets_.push_back(std::move(net));
}


//------------------------------------------------------------------------------
double RGA2DProblem::connectorWeights(const std::size_t id) const
{
    double weight = 0.0;
    for (const auto net : objNets_[id])
        weight += nets_[net].weight;
    return weight;
}


//------------------------------------------------------------------------------
void RGA2DProblem::init()
{
    if ((width_ <= 0) || (height_ <= 0))
        mThrowRException("The limits of the placement are not defined");
    if (objs_.empty())
        mThrowRException("The problem contains no object");

    // Configurations of the objects
    for (auto& obj : objs_)
    {
        obj->initConfigs();
        const auto fits = std::any_of(obj->configs().begin(), obj->configs().end(), [this](const RGA2DConfig& cfg) {
            return (cfg.width <= width_) && (cfg.height <= height_);
        });
        if (!fits)
            mThrowRException("Object '", obj->name(), "' does not fit in the limits ", width_, "x", height_);
    }

    // Nets of each object (a net is counted once per object)
    objNets_.assign(objs_.size(), {});
    for (std::size_t net = 0; net < nets_.size(); ++net)
        for (const auto& item : nets_[net].items)
            if ((item.obj != cNoRef) && (std::find(objNets_[item.obj].begin(), objNets_[item.obj].end(), net) == objNets_[item.obj].end()))
                objNets_[item.obj].push_back(net);

    // Centers of the pins of the board
    boardCenters_.clear();
    for (const auto& con : board_)
    {
        std::vector<RPoint> centers;
        for (const auto& pin : con.pins())
            centers.push_back(pin.rect.center());
        boardCenters_.push_back(std::move(centers));
    }
}
