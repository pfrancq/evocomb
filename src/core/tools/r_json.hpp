/*

    EvoComb

    RJSON.hpp

    JSON node - Header.

    Copyright 1999-2026 by Pascal Francq (pascal@francq.info).
    Copyright 1999-2008 by the Université Libre de Bruxelles (ULB).

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
#include <map>
#include <memory>
#include <string>
#include <variant>
#include <vector>


//------------------------------------------------------------------------------
namespace evocomb{
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// forward declaration
class RJSONParser;


//------------------------------------------------------------------------------
enum class RJSONType
{
    Null,
    String,
    Decimal,
    Integer,
    Boolean,
    Array,
    Object,
};


//------------------------------------------------------------------------------
std::string to_string(RJSONType type);


//------------------------------------------------------------------------------
class RJSON
{
public:

    /**
     * Copy constructor.
     * @param json          JSON element.
     */
    RJSON(const RJSON& json);

    /**
     * Create a default element (numbers are set to 0, boolean value to false).
     * @param type              Type.
     */
    explicit RJSON(RJSONType type = RJSONType::Object);

    /**
     * Create a JSON string.
     * @param value             Value.
     */
    explicit RJSON(const std::string& value);

    /**
     * Create a JSON string.
     * @param value             Value.
     */
    explicit RJSON(const char* value);

    /**
     * Create a JSON decimal.
     * @param value             Value.
     */
    explicit RJSON(float value);

    /**
     * Create a JSON decimal.
     * @param value             Value.
     */
    explicit RJSON(double value);

    /**
     * Create a JSON integer.
     * @param value             Value.
     */
    explicit RJSON(int value);

    /**
     * Create a JSON integer.
     * @param value             Value.
     */
    explicit RJSON(long int value);

    /**
     * Create a JSON integer.
     * @param value             Value.
     */
    explicit RJSON(long long int value);

    /**
     * Create a JSON boolean.
     * @param value             Value.
     */
    explicit RJSON(bool value);

    /**
     * Destructor.
     */
    ~RJSON();

    /**
     * Get the type.
     * @return a RJSONType value.
     */
    [[nodiscard]] RJSONType type() const { return type_; }

    /**
     * Look if the element is an object.
     * @return `true` if this is the case, `false` otherwise.
     */
    [[nodiscard]] bool isObject() const { return type_ == RJSONType::Object; }

    /**
     * Look if the element is an array.
     * @return `true` if this is the case, `false` otherwise.
     */
    [[nodiscard]] bool isArray() const { return type_ == RJSONType::Array; }

    /**
     * Look if the element is null.
     * @return `true` if this is the case, `false` otherwise.
     */
    [[nodiscard]] bool isNull() const { return type_ == RJSONType::Null; }

    /**
     * Look if the element is a string.
     * @return `true` if this is the case, `false` otherwise.
     */
    [[nodiscard]] bool isString() const { return type_ == RJSONType::String; }

    /**
     * Look if the element is a decimal.
     * @return `true` if this is the case, `false` otherwise.
     */
    [[nodiscard]] bool isDecimal() const { return type_ == RJSONType::Decimal; }

    /**
     * Look if the element is an integer.
     * @return `true` if this is the case, `false` otherwise.
     */
    [[nodiscard]] bool isInteger() const { return type_ == RJSONType::Integer; }

    /**
     * Look if the element is a boolean.
     * @return `true` if this is the case, `false` otherwise.
     */
    [[nodiscard]] bool isBoolean() const { return type_ == RJSONType::Boolean; }

    /**
     * Get the key.
     * @return a string.
     */
    [[nodiscard]] std::string key() const { return key_; }

    /**
     * Get the number of elements of an object or an array (0 for the other types).
     */
    [[nodiscard]] std::size_t size() const;

    /**
     * Get a pointer to the parent.
     * @return `nullptr` if no parent defined, a constant valid pointe otherwise.
     */
    [[nodiscard]] const RJSON* parent() const { return parent_; }

    /**
     * Get a pointer to the parent.
     * @return `nullptr` if no parent defined, a valid pointe otherwise.
     */
    RJSON* parent() { return parent_; }

    /**
     * Get a string value.
     * @return a string.
     */
    [[nodiscard]] std::string toString() const;

    /**
     * Get a decimal value.
     * @return double.
     */
    [[nodiscard]] double toDecimal() const;

    /**
     * Get an integer value.
     * @return long long int.
     */
    [[nodiscard]] long long int toInteger() const;

    /**
     * Get a boolean value.
     * @return bool.
     */
    [[nodiscard]] bool toBool() const;

    /**
     * Get the ith element of a JSON array or object (the elements of an object are in their order of insertion, and
     * their key is given by key). If the index is invalid or the instance isn't an array or an object, an exception is
     * thrown.
     * @param idx           Index of the element.
     * @return a constant reference to a JSON element.
     */
    const RJSON& operator[](size_t idx) const;

    /**
     * Get the ith element of a JSON array or object (the elements of an object are in their order of insertion, and
     * their key is given by key). If the index is invalid or the instance isn't an array or an object, an exception is
     * thrown.
     * @param idx           Index of the element.
     * @return reference to a JSON element.
     */
    RJSON& operator[](size_t idx);

    /**
     * Get the element with a given key in a JSON object. If the key is invalid or the instance isn't an object, an
     * exception is thrown.
     * @param key           Key of the element.
     * @return a constant reference to a JSON element.
     */
    const RJSON& operator[](const std::string& key) const;

    /**
     * Get the element with a given key in a JSON object. If the key is invalid or the instance isn't an object, an
     * exception is thrown.
     * @param key           Key of the element.
     * @return a reference to a JSON element.
     */
    RJSON& operator[](const std::string& key);

    /**
     * Search for an element with a given key.
     * @param key           Key to search for. If the element is an array, the key is supposed to be the index.
     * @return `nullptr` if the element wasn't found, a valid constant pointer otherwise.
     */
    [[nodiscard]] const RJSON* find(const std::string& key) const;

    /**
     * Search for an element with a given key.
     * @param key           Key to search for. If the element is an array, the key is supposed to be the index.
     * @return `nullptr` if the element wasn't found, a valid pointer otherwise.
     */
    [[nodiscard]] RJSON* find(const std::string& key);

    /**
     * Search for an element provided by a series of keys.
     * @param keys          Keys to search for. If one of the element is an array, the key is supposed to be the index.
     * @return `nullptr` if the element wasn't found, a valid constant pointer otherwise.
     */
    [[nodiscard]] const RJSON* find(const std::vector<std::string>& keys) const;

    /**
     * Search for an element provided by a series of keys.
     * @param keys          Keys to search for. If one of the element is an array, the key is supposed to be the index.
     * @return `nullptr` if the element wasn't found, a valid pointer otherwise.
     */
    [[nodiscard]] RJSON* find(const std::vector<std::string>& keys);

    /**
     * Add a JSON element to an array. Its key becomes (a string representation of) its index.
     * @param json              JSON element (will be moved).
     * @return a pointer to the element inserted.
     */
    RJSON* add(std::unique_ptr<RJSON>&& json);

    /**
     * Add a JSON element to an object. If an element already exists with that key, it is replaced.
     * @param key               Key of the element (will be moved).
     * @param json              JSON element (will be moved).
     * @return a pointer to the element inserted.
     */
    RJSON* add(const std::string& key, std::unique_ptr<RJSON>&& json);

    /**
     * Merge a JSON object into the current one: each element of json replaces the element with the same key, except if
     * both are objects, in which case they are merged recursively.
     * @code
     * auto config = RJSON::parse(R"({"ga": {"seed": 1, "nb_threads": 1}})");
     * config.merge(RJSON::parse(R"({"ga": {"nb_threads": 4}})"));
     * // {"ga": {"seed": 1, "nb_threads": 4}}
     * @endcode
     * @param json              JSON object to merge.
     * @exception RException    One of the elements is not an object.
     */
    void merge(const RJSON& json);

    /**
     * Remove (and delete) an element given by a given key (if exists).
     * @param key               Key to search for. If the element is an array, the key is supposed to be the index.
     */
    void remove(const std::string& key);

    /**
     * Remove (and delete) an element from an array.
     * @param idx               Index of the element.
     */
    void remove(size_t idx);

    /**
     * Maximal width of a line in an indented dump (see dump).
     */
    static constexpr std::size_t MaxLineWidth = 120;

    /**
     * Dump the JSON element into a string.
     * @param tabSize           Number of spaces forming the tab to indent per depth. If 0 (default value), a compact
     *                          version is generated. Otherwise, an object or an array is written on a single line if it
     *                          fits in MaxLineWidth characters (indentation included).
     * @return a string.
     */
    [[nodiscard]] std::string dump(int tabSize = 0) const;

    /**
     * Parse a string.
     * @param json          JSON string.
     * @return a RJSON instance.
     */
    static RJSON parse(const std::string& json);

private:

    /**
     * Remove all the references of a given element. In case of an array, the keys are renamed.
     * @param json              Element to remove.
     */
    void removeRef(const RJSON* json);

    /**
     * Dump the current element (and its children) in a given string.
     * @param tab               String representing a tab.
     * @param depth             Current depth.
     * @param result            Resulting string where the element must be added.
     */
    void dump(const std::string& tab, int depth, std::string& result) const;

    /**
     * Dump the current element (and its children) on a single line, with a space after the commas and the colons.
     * @param result            Resulting string where the element must be added.
     */
    void dumpInline(std::string& result) const;

    /**
     * Append the value of a scalar element (nothing for an object or an array).
     * @param result            Resulting string.
     */
    void appendScalar(std::string& result) const;

    /**
     * Append a string, with its quotes and its special characters escaped.
     * @param str               String.
     * @param result            Resulting string.
     */
    static void appendString(const std::string& str, std::string& result);

    /**
     * Append a decimal value with enough digits to read back the same value (null if the value is infinite or NaN).
     * @param value             Value.
     * @param result            Resulting string.
     */
    static void appendDecimal(double value, std::string& result);

    //-----------------------------------------------------
    struct Items
    {
        std::vector<std::unique_ptr<RJSON>> children_;
        std::map<std::string, RJSON*> map_;
    };

    using Value = std::variant<
        std::monostate,
        std::string,
        double,
        long long int,
        bool,
        Items
    >;

    //-----------------------------------------------------
    RJSONType type_;
    RJSON* parent_ = nullptr;
    std::string key_;
    // std::map<std::string, RJSON*> childrenByName_;
    // std::vector<std::unique_ptr<RJSON>> children_;
    Value value_;

    //-----------------------------------------------------
    friend class RJSONParser;
};



}  //-------- End of namespace evocomb ---------------------------------------------
