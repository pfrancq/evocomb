/*

    EvoComb

    RGA2DProblem.hpp

    2D Placement Problem - Header.

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
#include <vector>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "geometry/r_point.hpp"
#include "geometry/r_polygon.hpp"
#include "geometry/r_rect.hpp"
#include "tools/r_std.hpp"


//------------------------------------------------------------------------------
namespace evocomb {
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
 * The RGA2DPin class provides a pin of a connector: a rectangle where a connection can be done. The rectangle is
 * expressed in the frame of the object in its normal orientation (or in absolute coordinates for a connector of the
 * board).
 * @short 2D Pin.
 */
struct RGA2DPin
{
    std::string name;                       ///< Name.
    std::string layer;                      ///< Layer (informative).
    RRect rect;                             ///< Rectangle.
};


//------------------------------------------------------------------------------
/**
 * The RGA2DConnector class provides a connector (a "terminal") of an object or of the board: a set of pins which are
 * electrically equivalent. A net connected to the connector can use any of its pins.
 * @short 2D Connector.
 */
class RGA2DConnector
{
public:

    /**
     * Construct the connector.
     * @param obj                Identifier of the object (cNoRef for the board).
     * @param id                 Index of the connector in its owner.
     * @param name               Name.
     */
    RGA2DConnector(std::size_t obj, std::size_t id, std::string name);

    /**
     * Get the identifier of the object owning the connector (cNoRef for the board).
     */
    [[nodiscard]] std::size_t obj() const { return obj_; }

    /**
     * Get the index of the connector in its owner.
     */
    [[nodiscard]] std::size_t id() const { return id_; }

    /**
     * Get the name.
     */
    [[nodiscard]] const std::string& name() const { return name_; }

    /**
     * Get the pins.
     */
    [[nodiscard]] const std::vector<RGA2DPin>& pins() const { return pins_; }

    /**
     * Add a pin.
     * @param pin                Pin.
     */
    void addPin(RGA2DPin pin) { pins_.push_back(std::move(pin)); }

private:

    //-----------------------------------------------------
    std::size_t obj_;                       ///< Owner.
    std::size_t id_;                        ///< Index in the owner.
    std::string name_;                      ///< Name.
    std::vector<RGA2DPin> pins_;            ///< Pins.
};


//------------------------------------------------------------------------------
/**
 * The RGA2DConfig class provides a configuration of an object: its shape in a given orientation (translated so that its
 * bounding rectangle starts at (0,0)), decomposed into rectangles, and the positions of the pins of its connectors.
 * @short 2D Object Configuration.
 */
struct RGA2DConfig
{
    ROrientation orientation = ROrientation::Normal;    ///< Orientation.
    RPolygon polygon;                       ///< Shape.
    std::vector<RRect> rects;               ///< Decomposition of the shape.
    RCoord width = 0;                       ///< Width of the bounding rectangle.
    RCoord height = 0;                      ///< Height of the bounding rectangle.
    RCoord area = 0;                        ///< Area of the shape.
    std::vector<std::vector<RRect>> pins;   ///< Rectangles of the pins of each connector.
    std::vector<std::vector<RPoint>> centers;   ///< Centers of the pins of each connector.
};


//------------------------------------------------------------------------------
/**
 * The RGA2DObj class provides an object to place: a rectilinear shape (in its normal orientation) with connectors. Once
 * all its connectors and pins are defined, the configurations (one per distinct orientation) are computed by
 * RGA2DProblem::init.
 * @short 2D Object.
 */
class RGA2DObj
{
public:

    /**
     * Construct the object.
     * @param id                 Identifier (its index in the problem).
     * @param name               Name.
     * @param polygon            Shape (it is translated so that its bounding rectangle starts at (0,0); the pins must
     *                           be expressed in the same frame as the given polygon).
     */
    RGA2DObj(std::size_t id, std::string name, const RPolygon& polygon);

    /**
     * Get the identifier.
     */
    [[nodiscard]] std::size_t id() const { return id_; }

    /**
     * Get the name.
     */
    [[nodiscard]] const std::string& name() const { return name_; }

    /**
     * Get the shape in its normal orientation.
     */
    [[nodiscard]] const RPolygon& polygon() const { return polygon_; }

    /**
     * Add a connector.
     * @param name               Name.
     * @return the connector.
     */
    RGA2DConnector& addConnector(const std::string& name);

    /**
     * Add a pin to a connector. The rectangle is translated like the shape.
     * @param connector          Index of the connector.
     * @param pin                Pin.
     */
    void addPin(std::size_t connector, RGA2DPin pin);

    /**
     * Get the connectors.
     */
    [[nodiscard]] const std::vector<RGA2DConnector>& connectors() const { return connectors_; }

    /**
     * Find a connector by its name.
     * @param name               Name.
     * @return cNoRef if the connector does not exist.
     */
    [[nodiscard]] std::size_t findConnector(const std::string& name) const;

    /**
     * Get the configurations (the first one corresponds to the normal orientation).
     */
    [[nodiscard]] const std::vector<RGA2DConfig>& configs() const { return configs_; }

    /**
     * Compute the configurations: one per orientation giving a different shape or different positions of the pins.
     * @exception RException     The shape is not rectilinear.
     */
    void initConfigs();

private:

    //-----------------------------------------------------
    std::size_t id_;                        ///< Identifier.
    std::string name_;                      ///< Name.
    RPolygon polygon_;                      ///< Shape (normal orientation).
    RPoint translation_;                    ///< Translation applied to the shape and the pins.
    std::vector<RGA2DConnector> connectors_;    ///< Connectors.
    std::vector<RGA2DConfig> configs_;      ///< Configurations.
};


//------------------------------------------------------------------------------
/**
 * The RGA2DNet class provides a net: a set of connectors (of objects or of the board) that must be connected together,
 * with a weight.
 * @short 2D Net.
 */
struct RGA2DNet
{
    /**
     * A connector of the net.
     */
    struct Item
    {
        std::size_t obj;                    ///< Object (cNoRef for the board).
        std::size_t connector;              ///< Index of the connector.
    };

    std::string name;                       ///< Name.
    double weight = 1.0;                    ///< Weight.
    std::vector<Item> items;                ///< Connectors.
};


//------------------------------------------------------------------------------
/**
 * The RGA2DProblem class provides a 2D placement problem: some objects, with connectors, must be placed without
 * overlapping in a rectangular area [0,width[ x [0,height[ (the "limits"), so that the bounding rectangle of the
 * placement and the length of the nets are minimized. The board can also have connectors (outside the limits).
 * @code
 * evocomb::RGA2DProblem problem;
 * problem.limits(100, 100);
 * auto& obj = problem.addObj("A", evocomb::RPolygon(evocomb::RRect(0, 0, 10, 10)));
 * obj.addConnector("in");
 * obj.addPin(0, {"p1", "", evocomb::RRect(0, 4, 1, 5)});
 * // ...
 * problem.addNet({"n1", 1.0, {{0, 0}, {1, 0}}});
 * problem.init();
 * @endcode
 * @short 2D Placement Problem.
 */
class RGA2DProblem
{
public:

    /**
     * Construct an empty problem.
     */
    RGA2DProblem() = default;

    /**
     * Set the limits of the placement area.
     * @param width              Width.
     * @param height             Height.
     */
    void limits(RCoord width, RCoord height);

    /**
     * Get the width of the placement area.
     */
    [[nodiscard]] RCoord width() const { return width_; }

    /**
     * Get the height of the placement area.
     */
    [[nodiscard]] RCoord height() const { return height_; }

    /**
     * Add an object.
     * @param name               Name.
     * @param polygon            Shape (see RGA2DObj).
     * @return the object.
     */
    RGA2DObj& addObj(const std::string& name, const RPolygon& polygon);

    /**
     * Get the number of objects.
     */
    [[nodiscard]] std::size_t nbObjs() const { return objs_.size(); }

    /**
     * Get an object.
     * @param id                 Identifier.
     */
    [[nodiscard]] const RGA2DObj& obj(const std::size_t id) const { return *objs_[id]; }

    /**
     * Get an object.
     * @param id                 Identifier.
     */
    [[nodiscard]] RGA2DObj& obj(const std::size_t id) { return *objs_[id]; }

    /**
     * Find an object by its name.
     * @param name               Name.
     * @return cNoRef if the object does not exist.
     */
    [[nodiscard]] std::size_t findObj(const std::string& name) const;

    /**
     * Add a connector to the board.
     * @param name               Name.
     * @return the connector (its pins are in absolute coordinates).
     */
    RGA2DConnector& addBoardConnector(const std::string& name);

    /**
     * Get the connectors of the board.
     */
    [[nodiscard]] const std::vector<RGA2DConnector>& boardConnectors() const { return board_; }

    /**
     * Find a connector of the board by its name.
     * @param name               Name.
     * @return cNoRef if the connector does not exist.
     */
    [[nodiscard]] std::size_t findBoardConnector(const std::string& name) const;

    /**
     * Add a net.
     * @param net                Net.
     * @exception RException     A connector does not exist.
     */
    void addNet(RGA2DNet net);

    /**
     * Get the nets.
     */
    [[nodiscard]] const std::vector<RGA2DNet>& nets() const { return nets_; }

    /**
     * Get the nets connected to an object.
     * @param id                 Identifier of the object.
     */
    [[nodiscard]] const std::vector<std::size_t>& objNets(const std::size_t id) const { return objNets_[id]; }

    /**
     * Get the total weight of the nets connected to an object.
     * @param id                 Identifier of the object.
     */
    [[nodiscard]] double connectorWeights(std::size_t id) const;

    /**
     * Get the centers of the pins of a connector of the board (absolute coordinates).
     * @param connector          Index of the connector.
     */
    [[nodiscard]] const std::vector<RPoint>& boardCenters(const std::size_t connector) const { return boardCenters_[connector]; }

    /**
     * Initialize the problem once all the elements are defined: computation of the configurations of the objects and of
     * the nets connected to each object.
     * @exception RException     The problem is invalid (no limits, an object that cannot fit in the limits in any
     *                           orientation, etc.).
     */
    void init();

    //-----------------------------------------------------
    RGA2DProblem(const RGA2DProblem&) = delete;
    RGA2DProblem& operator=(const RGA2DProblem&) = delete;

private:

    //-----------------------------------------------------
    RCoord width_ = 0;                      ///< Width of the placement area.
    RCoord height_ = 0;                     ///< Height of the placement area.
    std::vector<std::unique_ptr<RGA2DObj>> objs_;   ///< Objects.
    std::vector<RGA2DConnector> board_;     ///< Connectors of the board.
    std::vector<std::vector<RPoint>> boardCenters_; ///< Centers of the pins of the board.
    std::vector<RGA2DNet> nets_;            ///< Nets.
    std::vector<std::vector<std::size_t>> objNets_; ///< Nets of each object.
};


}  //------- End of namespace evocomb ----------------------------------------------
