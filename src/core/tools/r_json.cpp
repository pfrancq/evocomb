/*

    EvoComb

    RJSON.cpp

    JSON node - Implementation.

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
// include files for ANSI C/C++
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "tools/r_json.hpp"
#include "tools/r_exception.hpp"
#include "app/r_logger.hpp"
#include "io/r_json_parser.hpp"
using namespace evocomb;


//------------------------------------------------------------------------------
mLogModule("r.core");



//------------------------------------------------------------------------------
//
// General functions
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
std::string evocomb::to_string(const RJSONType type)
{
    switch (type)
    {
        case RJSONType::Object:
            return "Object";
        case RJSONType::Array:
            return "Array";
        case RJSONType::String:
            return "String";
        case RJSONType::Decimal:
        return "Decimal";
        case RJSONType::Integer:
            return "Integer";
        case RJSONType::Boolean:
            return "boolean";
        case RJSONType::Null:
            return "null";
    }
    mThrowRException("Invalid type");
}



//------------------------------------------------------------------------------
//
// class JSON
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
RJSON::RJSON(const RJSON& json):
    type_(json.type_),
    key_(json.key_)
{
    switch (type_)
    {
        case RJSONType::Object:
            value_ = Items();
            for (const auto& child: std::get<Items>(json.value_).children_)
                add(child->key_, std::make_unique<RJSON>(*child));
            break;
        case RJSONType::Array:
            value_ = Items();
            for (const auto& child: std::get<Items>(json.value_).children_)
                add(std::make_unique<RJSON>(*child));
            break;
        case RJSONType::String:
            value_ = std::get<std::string>(json.value_);
            break;
        case RJSONType::Decimal:
            value_ = std::get<double>(json.value_);
            break;
        case RJSONType::Integer:
            value_ = std::get<long long int>(json.value_);
            break;
        case RJSONType::Boolean:
            value_ = std::get<bool>(json.value_);
            break;
        default:
            break;
    }
}


//------------------------------------------------------------------------------
RJSON::RJSON(RJSONType type):
    type_(type)
{
    switch (type)
    {
        case RJSONType::Object:
        case RJSONType::Array:
            value_ = Items();
            break;
        case RJSONType::Decimal:
            value_  = 0.0;
            break;
        case RJSONType::Integer:
            value_  = 0;
            break;
        case RJSONType::Boolean:
            value_ = false;
            break;
        default:
            break;
    }
}


//------------------------------------------------------------------------------
RJSON::RJSON(const std::string& value):
    type_(RJSONType::String),
    value_(value)
{
}


//------------------------------------------------------------------------------
RJSON::RJSON(const char* value):
    type_(RJSONType::String),
    value_(std::string(value))
{
}


//------------------------------------------------------------------------------
RJSON::RJSON(const float value):
    type_(RJSONType::Decimal),
    value_(static_cast<double>(value))
{
}


//------------------------------------------------------------------------------
RJSON::RJSON(double value):
    type_(RJSONType::Decimal),
    value_(value)
{
}


//------------------------------------------------------------------------------
RJSON::RJSON(const int value):
    type_(RJSONType::Integer),
    value_(static_cast<long long>(value))
{
}


//------------------------------------------------------------------------------
RJSON::RJSON(long int value):
    type_(RJSONType::Integer),
    value_(static_cast<long long>(value))
{
}


//------------------------------------------------------------------------------
RJSON::RJSON(long long int value):
    type_(RJSONType::Integer),
    value_(value)
{
}


//------------------------------------------------------------------------------
RJSON::RJSON(bool value):
    type_(RJSONType::Boolean),
    value_(value)
{
}


//------------------------------------------------------------------------------
RJSON::~RJSON()
{
    if (type_ == RJSONType::Object || type_ == RJSONType::Array)
    {
        // Set the parent to nullptr to avoid a double deletion
        for (const auto& child: std::get<Items>(value_).children_)
            child->parent_ = nullptr;

        // If a parent -> Remove the reference
        if (parent_ != nullptr)
            parent_->removeRef(this);
    }
}


//------------------------------------------------------------------------------
std::string RJSON::toString() const
{
    if (type_ != RJSONType::String)
        mThrowRException("Not a string element");
    return std::get<std::string>(value_);
}


//------------------------------------------------------------------------------
double RJSON::toDecimal() const
{
    if (type_ != RJSONType::Decimal)
        mThrowRException("Not a decimal element");
    return std::get<double>(value_);
}


//------------------------------------------------------------------------------
long long int RJSON::toInteger() const
{
    if (type_ != RJSONType::Integer)
        mThrowRException("Not a integer element");
    return std::get<long long int>(value_);
}


//------------------------------------------------------------------------------
bool RJSON::toBool() const
{
    if (type_ != RJSONType::Boolean)
        mThrowRException("Not a bool element");
    return std::get<bool>(value_);
}


//------------------------------------------------------------------------------
const RJSON& RJSON::operator[](const size_t idx) const
{
    if (type_ != RJSONType::Array && type_ != RJSONType::Object)
        mThrowRException("operator[size_t] works only with array or object");
    const auto& array = std::get<Items>(value_).children_;
    if (idx >= array.size())
        mThrowRException("Invalid " + std::to_string(idx) + " index");
    return *array[idx];
}


//------------------------------------------------------------------------------
RJSON& RJSON::operator[](const size_t idx)
{
    if (type_ != RJSONType::Array && type_ != RJSONType::Object)
        mThrowRException("operator[size_t] works only with array or object");
    const auto& array = std::get<Items>(value_).children_;
    if (idx >= array.size())
        mThrowRException("Invalid " + std::to_string(idx) + " index");
    return *array[idx];
}


//------------------------------------------------------------------------------
const RJSON& RJSON::operator[](const std::string& key) const
{
    if (type_ != RJSONType::Object)
        mThrowRException("operator[std::string] works only with object");
    const auto& map = std::get<Items>(value_).map_;
    const auto find = map.find(key);
    if (find == map.end())
        mThrowRException("Invalid " + key + " key");
    return *find->second;
}


//------------------------------------------------------------------------------
RJSON& RJSON::operator[](const std::string& key)
{
    if (type_ != RJSONType::Object)
        mThrowRException("operator[std::string] works only with object");
    const auto& map = std::get<Items>(value_).map_;
    const auto find = map.find(key);
    if (find == map.end())
        mThrowRException("Invalid " + key + " key");
    return *find->second;
}



//------------------------------------------------------------------------------
const RJSON* RJSON::find(const std::string& key) const
{
    return const_cast<RJSON*>(this)->find(key);
}


//------------------------------------------------------------------------------
RJSON* RJSON::find(const std::string& key)
{
    if (type_ != RJSONType::Object && type_ != RJSONType::Array)
        return nullptr;
    const auto& array = std::get<Items>(value_).children_;
    if (array.empty())
        return nullptr;
    if (type_ == RJSONType::Array)
    {
        size_t idx;
        if (auto [ptr, ec] = std::from_chars(key.data(), key.data() + key.size(), idx); ec != std::errc())
            return nullptr;
        if (idx >= array.size())
            return nullptr;
        return array[idx].get();
    }
    const auto& map = std::get<Items>(value_).map_;
    const auto find = map.find(key);
    if (find == map.end())
        return nullptr;
    return find->second;
}


//------------------------------------------------------------------------------
const RJSON* RJSON::find(const std::vector<std::string>& keys) const
{
    return const_cast<RJSON*>(this)->find(keys);
}


//------------------------------------------------------------------------------
RJSON* RJSON::find(const std::vector<std::string>& keys)
{
    if (type_ != RJSONType::Object && type_ != RJSONType::Array)
        return nullptr;
    auto cur = this;
    for (const auto& key : keys)
    {
        cur = find(key);
        if (cur == nullptr)
            break;
    }
    return cur;
}


//------------------------------------------------------------------------------
RJSON* RJSON::add(std::unique_ptr<RJSON>&& json)
{
    if (type_ != RJSONType::Array)
        mThrowRException("Not an array element");
    auto& [array, map] = std::get<Items>(value_);
    if (type_ == RJSONType::Array)
        json->key_ = std::to_string(array.size());
    array.emplace_back(std::move(json));
    auto key = array.back()->key_;
    map.emplace(key, array.back().get());
    array.back()->parent_ = this;
    return array.back().get();
}


//------------------------------------------------------------------------------
RJSON* RJSON::add(const std::string& key, std::unique_ptr<RJSON>&& json)
{
    if (type_ != RJSONType::Object)
        mThrowRException("Not object");
    auto& [array, map] = std::get<Items>(value_);
    if (key.empty())
        mThrowRException("Cannot insert an element with a empty key into an Object");
    remove(key);
    json->key_ = key;
    array.emplace_back(std::move(json));
    map.emplace(key, array.back().get());
    array.back()->parent_ = this;
    return array.back().get();
}


//------------------------------------------------------------------------------
void RJSON::remove(const std::string& key)
{
    if (type_ == RJSONType::Array)
    {
        const auto& array = std::get<Items>(value_).children_;
        size_t idx;
        if (auto [ptr, ec] = std::from_chars(key.data(), key.data() + key.size(), idx); ec != std::errc())
            return;
        if (idx >= array.size())
            return;
        const auto ptr = array[idx].get();
        removeRef(ptr);
        delete ptr;
    }
    else
    {
        const auto& map = std::get<Items>(value_).map_;
        const auto find = map.find(key);
        if (find == map.end())
            return;
        const auto ptr = find->second;
        removeRef(ptr);
        delete ptr;
    }
}


//------------------------------------------------------------------------------
void RJSON::remove(const size_t idx)
{
    if (type_ != RJSONType::Array)
        return;
    const auto& array = std::get<Items>(value_).children_;
    if (idx >= array.size())
        return;
    const auto ptr = array[idx].get();
    removeRef(ptr);
    delete ptr;
}


//------------------------------------------------------------------------------
std::string RJSON::dump(const int tabSize) const
{
    std::string result;
    std::string tabs;
    tabs.reserve(tabSize * 20);
    for (auto i =  0; i < tabSize; i++)
        tabs += ' ';
    result.reserve(1024);
    if (type_ == RJSONType::Array || type_ == RJSONType::Object)
        dump(tabs, 0, result);
    else
    {
        // A value dumped alone is written as an object
        result += '{';
        if (tabSize > 0)
            result += '\n';
        dump(tabs, 1, result);
        if (tabSize > 0)
            result += '\n';
        result += '}';
    }
    return result;
}


//------------------------------------------------------------------------------
RJSON RJSON::parse(const std::string& json)
{
    RJSON result;
    RJSONParser parser;
    parser.populate(&result);
    parser.parse(json.c_str(), json.length());
    return result;
}


//------------------------------------------------------------------------------
void RJSON::removeRef(const RJSON* json)
{
    auto& [array, map] = std::get<Items>(value_);
    map.erase(json->key_);
    auto iter = array.begin();
    auto idx = 0;
    auto found = false;
    while (iter != array.end())
    {
        if (iter->get() == json)
        {
            iter->release(); // Release ownership
            iter = array.erase(iter);
            if (type_ == RJSONType::Object)
                break;          // If Object -> Can stop here
            // Array -> Rename the index of the next elements.
            found = true;
        }
        if (found)
            iter->get()->key_ = std::to_string(idx);
        idx++;
        ++iter;
    }
}


//------------------------------------------------------------------------------
void RJSON::dump(const std::string& tab, const int depth, std::string& result) const
{
    const auto pretty = !tab.empty();
    const auto indent = [&](const int level) {
        for (auto i = 0; i < level; i++)
            result += tab;
    };
    if (pretty)
        indent(depth);

    // The key is written for the children (and for a value dumped alone, which is written as an object), but not for an
    // object or an array dumped alone.
    const bool alone = depth == 0 && (type_ == RJSONType::Object || type_ == RJSONType::Array);
    if (!alone && parent_ != nullptr && parent_->type_ != RJSONType::Array)
    {
        appendString(key_, result);
        result += pretty ? ": " : ":";
    }
    if (type_ != RJSONType::Object && type_ != RJSONType::Array)
    {
        appendScalar(result);
        return;
    }

    const auto isObject = (type_ == RJSONType::Object);
    const auto& children = std::get<Items>(value_).children_;
    if (pretty && (!children.empty()))
    {
        // An object or an array that fits on the current line (with a possible comma) is written on it
        std::string line;
        dumpInline(line);
        const auto lineStart = result.rfind('\n');
        const auto column = (lineStart == std::string::npos) ? result.size() : result.size() - lineStart - 1;
        if (column + line.size() + 1 <= MaxLineWidth)
        {
            result += line;
            return;
        }
    }
    result += isObject ? '{' : '[';
    if (!children.empty())
    {
        if (pretty)
            result += '\n';
        auto first = true;
        for (auto& child : children)
        {
            if (first)
                first = false;
            else
            {
                result += ',';
                if (pretty)
                    result += '\n';
            }
            child->dump(tab, depth + 1, result);
        }
        if (pretty)
        {
            result += '\n';
            indent(depth);
        }
    }
    result += isObject ? '}' : ']';
}


//------------------------------------------------------------------------------
void RJSON::dumpInline(std::string& result) const
{
    if (type_ != RJSONType::Object && type_ != RJSONType::Array)
    {
        appendScalar(result);
        return;
    }
    const auto isObject = type_ == RJSONType::Object;
    result += isObject ? '{' : '[';
    auto first = true;
    for (auto& child : std::get<Items>(value_).children_)
    {
        if (first)
            first = false;
        else
            result += ", ";
        if (isObject)
        {
            appendString(child->key_, result);
            result += ": ";
        }
        child->dumpInline(result);
    }
    result += isObject ? '}' : ']';
}


//------------------------------------------------------------------------------
void RJSON::appendScalar(std::string& result) const
{
    switch (type_)
    {
        case RJSONType::Null:
            result += "null";
            break;
        case RJSONType::Boolean:
            result += std::get<bool>(value_) ? "true" : "false";
            break;
        case RJSONType::Decimal:
            appendDecimal(std::get<double>(value_), result);
            break;
        case RJSONType::Integer:
            result += std::to_string(std::get<long long>(value_));
            break;
        case RJSONType::String:
            appendString(std::get<std::string>(value_), result);
            break;
        case RJSONType::Object:
        case RJSONType::Array:
            break;
    }
}


//------------------------------------------------------------------------------
std::size_t RJSON::size() const
{
    if (type_ != RJSONType::Object && type_ != RJSONType::Array)
        return 0;
    return std::get<Items>(value_).children_.size();
}


//------------------------------------------------------------------------------
void RJSON::appendString(const std::string& str, std::string& result)
{
    static constexpr char hex[] = "0123456789abcdef";
    result += '"';
    for (const auto c : str)
    {
        switch (c)
        {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\b': result += "\\b"; break;
            case '\f': result += "\\f"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20)
                {
                    // Other control characters
                    result += "\\u00";
                    result += hex[(static_cast<unsigned char>(c) >> 4) & 0x0F];
                    result += hex[static_cast<unsigned char>(c) & 0x0F];
                }
                else
                    result += c;
        }
    }
    result += '"';
}


//------------------------------------------------------------------------------
void RJSON::appendDecimal(const double value, std::string& result)
{
    // JSON has no representation for infinite or NaN values
    if (!std::isfinite(value))
    {
        result += "null";
        return;
    }

    // Shortest representation (15 or 17 significant digits) that gives back the same value
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.15g", value);
    if (std::strtod(buf, nullptr) != value)
        std::snprintf(buf, sizeof(buf), "%.17g", value);
    std::string str(buf);

    // The value must be read back as a decimal
    if (str.find_first_of(".eE") == std::string::npos)
        str += ".0";
    result += str;
}


//------------------------------------------------------------------------------
void RJSON::merge(const RJSON& json)
{
    if (type_ != RJSONType::Object || json.type_ != RJSONType::Object)
        mThrowRException("merge works only with objects");
    for (const auto& child : std::get<Items>(json.value_).children_)
    {
        if (const auto cur = find(child->key_); cur != nullptr && cur->isObject() && child->isObject())
            cur->merge(*child);
        else
            add(child->key_, std::make_unique<RJSON>(*child));
    }
}
