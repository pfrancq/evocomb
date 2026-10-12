/*

    EvoComb

    RJSONParser.cpp

    Text File - Implementation.

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
// include files for ANSI C/C++
#include <cstdint>
#include <iostream>
#include <string>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "io/r_json_parser.hpp"
#include "tools/r_json.hpp"
#include "app/r_logger.hpp"
using namespace evocomb;


//------------------------------------------------------------------------------
mLogModule("r.core");



//------------------------------------------------------------------------------
//
// class RJSONParser
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
void RJSONParser::populate(RJSON* json)
{
    cur_ = json;
    isCurrentRoot_ = true;
}


//------------------------------------------------------------------------------
void RJSONParser::beginObject(const std::string& key)
{
    if (cur_ != nullptr)
    {
        if (isCurrentRoot_)
            isCurrentRoot_ = false;
        else
            add(key, std::make_unique<RJSON>(RJSONType::Object));
    }
}


//------------------------------------------------------------------------------
void RJSONParser::endObject(const std::string&)
{
    if (cur_ != nullptr)
        cur_ = cur_->parent();
}


//------------------------------------------------------------------------------
void RJSONParser::beginArray(const std::string& key)
{
    if (cur_ != nullptr)
    {
        if (isCurrentRoot_)
        {
            // The root is an array (objects and arrays share the same storage, only the type changes).
            isCurrentRoot_ = false;
            cur_->type_ = RJSONType::Array;
        }
        else
            add(key, std::make_unique<RJSON>(RJSONType::Array));
    }
}


//------------------------------------------------------------------------------
void RJSONParser::endArray(const std::string&)
{
    if (cur_ != nullptr)
        cur_ = cur_->parent();
}


//------------------------------------------------------------------------------
void RJSONParser::addString(const std::string& key, const std::string& value)
{
    add(key, std::make_unique<RJSON>(value));
}


//------------------------------------------------------------------------------
void RJSONParser::addDouble(const std::string& key, const double value)
{
    add(key, std::make_unique<RJSON>(value));
}


//------------------------------------------------------------------------------
void RJSONParser::addInteger(const std::string& key, const long value)
{
    add(key, std::make_unique<RJSON>(value));
}


//------------------------------------------------------------------------------
void RJSONParser::addBool(const std::string& key, const bool value)
{
    add(key, std::make_unique<RJSON>(value));
}


//------------------------------------------------------------------------------
void RJSONParser::addNull(const std::string& key)
{
    add(key, std::make_unique<RJSON>(RJSONType::Null));
}


//------------------------------------------------------------------------------
void RJSONParser::start()
{
    skipSpacesEol();
    if (cur() == '{')
    {
        beginObject("");
        readComplex();
        endObject("");
    }
    else if (cur() == '[')
    {
        beginArray("");
        readComplex();
        endArray("");
    }
    else
        mThrowRIOException(this, "Invalid character");
    skipSpacesEol();
    if (!end())
        mThrowRIOException(this, "'Invalid character");
}


//------------------------------------------------------------------------------
void RJSONParser::readComplex()
{
    const bool isObject = cur() == '{';
    next();     // Skip { or [
    while (true)
    {
        skipSpacesEol();
        if ((isObject && cur() == '}') || (!isObject && cur() == ']'))
            break;
        std::string key;
        if (isObject)
        {
            if (cur() != '"')
                mThrowRIOException(this, "'\"' expected");
            next();     // Skip "
            key = extractString();
            if (key.empty())
                mThrowRIOException(this, "Empty key");
            if (end())
                break;
            skipSpacesEol();
            if (cur() != ':')
                mThrowRIOException(this, "':' expected");
            next(); // Skip :
        }
        skipSpacesEol();
        if (cur() == '"')
        {
            next(); // Skip "
            addString(key, extractString());
        }
        else if (cur() == '[')
        {
            beginArray(key);
            readComplex();
            endArray(key);
        }
        else if (cur() == '{')
        {
            beginObject(key);
            readComplex();
            endObject(key);
        }
        else if (isStringNext("false", true, true))
        {
            addBool(key, false);
        }
        else if (isStringNext("true", true, true))
        {
            addBool(key, true);
        }
        else if (isStringNext("null", true, true))
        {
            addNull(key);
        }
        else
        {
            // Number?
            if (auto num = readToken(",}]"); !extractNumber(key, num))
                mThrowRIOException(this, "Invalid file");
        }
        skipSpacesEol();
        if ((isObject && cur() == '}') || (!isObject && cur() == ']'))
            break;
        if (cur() != ',')
            mThrowRIOException(this, "',' or '}' expected");
        next(); // Skip ,
    }
    next(); // Skip } or ]
    skipSpacesEol();
}


//------------------------------------------------------------------------------
std::string RJSONParser::extractString()
{
    // Append a code point encoded in UTF-8
    const auto appendUtf8 = [](std::string& str, const std::uint32_t code) {
        if (code < 0x80)
            str += static_cast<char>(code);
        else if (code < 0x800)
        {
            str += static_cast<char>(0xC0 | (code >> 6));
            str += static_cast<char>(0x80 | (code & 0x3F));
        }
        else if (code < 0x10000)
        {
            str += static_cast<char>(0xE0 | (code >> 12));
            str += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
            str += static_cast<char>(0x80 | (code & 0x3F));
        }
        else
        {
            str += static_cast<char>(0xF0 | (code >> 18));
            str += static_cast<char>(0x80 | ((code >> 12) & 0x3F));
            str += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
            str += static_cast<char>(0x80 | (code & 0x3F));
        }
    };

    // Read the four hexadecimal digits of a \u escape sequence
    const auto readHex = [this]() {
        std::uint32_t code = 0;
        for (int i = 0; i < 4; ++i)
        {
            next();
            if (end())
                mThrowRIOException(this, "Invalid \\u escape sequence");
            const auto c = cur();
            code <<= 4;
            if ((c >= '0') && (c <= '9'))
                code |= static_cast<std::uint32_t>(c - '0');
            else if ((c >= 'a') && (c <= 'f'))
                code |= static_cast<std::uint32_t>(c - 'a' + 10);
            else if ((c >= 'A') && (c <= 'F'))
                code |= static_cast<std::uint32_t>(c - 'A' + 10);
            else
                mThrowRIOException(this, "Invalid \\u escape sequence");
        }
        return code;
    };

    std::string res;
    while ((!end()) && (cur() != '"'))
    {
        if (cur() != '\\')
        {
            res += cur();
            next();
            continue;
        }

        // Escape sequence
        next();
        if (end())
            break;
        switch (cur())
        {
            case '"': res += '"'; break;
            case '\\': res += '\\'; break;
            case '/': res += '/'; break;
            case 'b': res += '\b'; break;
            case 'f': res += '\f'; break;
            case 'n': res += '\n'; break;
            case 'r': res += '\r'; break;
            case 't': res += '\t'; break;
            case 'u':
            {
                auto code = readHex();
                // Surrogate pair
                if ((code >= 0xD800) && (code <= 0xDBFF))
                {
                    next();
                    if (end() || (cur() != '\\'))
                        mThrowRIOException(this, "Invalid surrogate pair");
                    next();
                    if (end() || (cur() != 'u'))
                        mThrowRIOException(this, "Invalid surrogate pair");
                    const auto low = readHex();
                    if ((low < 0xDC00) || (low > 0xDFFF))
                        mThrowRIOException(this, "Invalid surrogate pair");
                    code = 0x10000 + ((code - 0xD800) << 10) + (low - 0xDC00);
                }
                appendUtf8(res, code);
                break;
            }
            default:
                mThrowRIOException(this, std::string("Invalid escape sequence '\\") + cur() + "'");
        }
        next();
    }
    next();     // Skip "
    return res;
}


//------------------------------------------------------------------------------
bool RJSONParser::extractNumber(const std::string& key, const std::string& s)
{
    try
    {
        // Check if it contains a decimal point or an exponent
        if (s.find_first_of(".eE") != std::string::npos)
        {
            // Decimal, try parsing a double
            size_t charProcessed;
            const double d = std::stod(s, &charProcessed);
            if (charProcessed != s.length())
                return false;
            addDouble(key, d);
        }
        else
        {
            // No decimal, try parsing as a long
            size_t charProcessed;
            const auto i = std::stol(s, &charProcessed);
            if (charProcessed != s.length())
                return false;
            addInteger(key, i);
        }
    }
    catch (...)
    {
        return false;
    }
    return true;
}


//------------------------------------------------------------------------------
void RJSONParser::add(const std::string& key, std::unique_ptr<RJSON>&& json)
{
    if (cur_ != nullptr)
    {
        RJSON* tmp;
        if (cur_->type() == RJSONType::Array)
        {
            if (!key.empty())
                mThrowRIOException(this, "Key not allowed in array");
            tmp = cur_->add(std::move(json));
        }
        else if (cur_->type() == RJSONType::Object)
            tmp = cur_->add(key, std::move(json));
        else
            mThrowRIOException(this, "Cannot insert into a " + to_string(cur_->type())  + "element");
        if (tmp->type() == RJSONType::Object || tmp->type() == RJSONType::Array)
            cur_ = tmp;
    }
}
