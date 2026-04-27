/*

    EvoComb

    RJSONParser.hpp

    Text File - Header.

    Copyright 2026 by Pascal Francq (pascal@francq.info).

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
#include <memory>
#include <string>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "io/r_text_parser.hpp"


//------------------------------------------------------------------------------
namespace evocomb{
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
class RJSON;


//------------------------------------------------------------------------------
/**
 * The RJSONParser implements a JSON parser. For each element parsed, the corresponding virtual methods are called. By
 * default, if the `setRootNode` method was used, the root node is populated.
 */
class RJSONParser : RTextParser
{
public:

    // Reuse the parse methods.
    using RTextParser::parse;

    /**
     * Constructor.
     */
    RJSONParser() = default;

    /**
     * Populate an element (it can only be an object or an array).
     * @param json              Pointer to an element. If `nullptr`, nothing is populate during parsing.
     */
    void populate(RJSON* json);

private:

    /**
     * Call each time a JSON object starts to be parsed.
     * @param key       Key of the object (can be empty for the root node or if the parent is an array).
     */
    virtual void beginObject(const std::string& key);

    /**
     * Call each time a JSON object ends to be parsed.
     * @param key       Key of the object (can be empty for the root node or if the parent is an array).
     */
    virtual void endObject(const std::string& key);

    /**
     * Call each time a JSON array starts to be parsed.
     * @param key       Key of the object (can be empty for the root node or if the parent is an array).
     */
    virtual void beginArray(const std::string& key);

    /**
     * Call each time a JSON array ends to be parsed.
     * @param key       Key of the object (can be empty for the root node or if the parent is an array).
     */
    virtual void endArray(const std::string& key);

    /**
     * Call each time a string value was parsed.
     * @param key       Key of the object (can be empty if the parent is an array).
     * @param value     Value.
     */
    virtual void addString(const std::string& key, const std::string& value);

    /**
     * Call each time a double value was parsed.
     * @param key       Key of the object (can be empty if the parent is an array).
     * @param value     Value.
     */
    virtual void addDouble(const std::string& key, double value);

    /**
     * Call each time a long value was parsed.
     * @param key       Key of the object (can be empty if the parent is an array).
     * @param value     Value.
     */
    virtual void addInteger(const std::string& key, long value);

    /**
     * Call each time a boolean value was parsed.
     * @param key       Key of the object (can be empty if the parent is an array).
     * @param value     Value.
     */
    virtual void addBool(const std::string& key, bool value);

    /**
     * Call each time a null value was parsed.
     * @param key       Key of the object (can be empty if the parent is an array).
     */
    virtual void addNull(const std::string& key);

    /**
     * Read a complex element (object or array).
     */
    void readComplex();

    /**
     * Start the parsing.
     */
    void start() override;

    /**
     * Exract a string by escaping \".
     * @return a string.
     */
    std::string extractString();

    /**
     * Extract a number (long or double).
     * @param key       Key of the object (can be empty if the parent is an array).
     * @param s         String representing the value.
     * @return `true` if a number could be read, `false` otherwise.
     */
    bool extractNumber(const std::string& key, const std::string& s);

    /**
     * Add a JSON element to the current object or array. If the element inserted is itself an object or array, the
     * current pointer is updated to point to it.
     * @param key       Key of the object (can be empty if the parent is an array).
     * @param json      JSON element to add.
     */
    void add(const std::string& key, std::unique_ptr<RJSON>&& json);

    //-----------------------------------------------------
    RJSON* cur_ = nullptr;                  // Current node to populate.
    bool isCurrentRoot_ = true;             // Is the current node the root node to populate?
};

}  //-------- End of namespace evocomb ---------------------------------------------
