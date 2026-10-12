/*

    EvoComb

    RLogger.cpp

    Logger - Implementation.

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
#include <cctype>
#include <chrono>
#include <iomanip>
#include <utility>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "app/r_logger.hpp"
#include "tools/r_exception.hpp"
#include "tools/r_json.hpp"
using namespace evocomb;


//------------------------------------------------------------------------------
mLogModule("r.core");


//------------------------------------------------------------------------------
namespace {

/**
 * Get a string parameter of the configuration.
 * @param parent                 Parent JSON object.
 * @param key                    Key of the parameter.
 * @return a pointer to the string or nullptr if the parameter is missing.
 */
const RJSON* findString(const RJSON& parent, const std::string& key)
{
    const auto param = parent.find(key);
    if (param != nullptr && !param->isString())
        mThrowRException("Logger configuration: '", key, "' must be a string");
    return param;
}

}  //------- End of anonymous namespace -----------------------------------------



//------------------------------------------------------------------------------
//
// class RLogger
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
RLogger::RLogger(const std::string& json)
{
    applyConfig(json, true);
}


//------------------------------------------------------------------------------
void RLogger::updateConfig(const std::string& json)
{
    applyConfig(json, false);
}


//------------------------------------------------------------------------------
std::filesystem::path RLogger::path() const
{
    const std::lock_guard lock(mutex_);
    return path_;
}


//------------------------------------------------------------------------------
void RLogger::applyConfig(const std::string& json, const bool initial)
{
    const auto config = RJSON::parse(json);

    // Read all the parameters in local variables (initialized with the current values) and verify them before modifying
    // the logger.
    const std::lock_guard lock(mutex_);
    auto level = this->level();
    auto screenLevel = this->screenLevel();
    auto screens = screens_;

    // File
    const auto path = findString(config, "path");
    if (initial && path == nullptr)
        mThrowRException("Logger configuration: missing 'path'");
    if (const auto param = findString(config, "level"); param != nullptr)
        level = toLevel(param->toString());
    else if (initial)
        mThrowRException("Logger configuration: missing 'level'");

    // Screen (optional)
    if (const auto screen = config.find("screen"); screen != nullptr)
    {
        if (!screen->isObject())
            mThrowRException("Logger configuration: 'screen' must be an object");
        if (const auto param = findString(*screen, "level"); param != nullptr)
            screenLevel = toLevel(param->toString());
        constexpr std::pair<const char*, RLogLevel> outputs[] = {
            {"error", RLogLevel::Error}, {"warning", RLogLevel::Warning}, {"info", RLogLevel::Info}, {"debug", RLogLevel::Debug}};
        for (const auto& [key, l] : outputs)
        {
            const auto output = findString(*screen, key);
            if (output == nullptr)
                continue;
            const auto name = output->toString();
            std::ostream* stream;
            if (name == "cerr")
                stream = &std::cerr;
            else if (name == "cout")
                stream = &std::cout;
            else
                mThrowRException("Logger configuration: invalid output '", name, "' for '", key, "' (\"cerr\" or \"cout\" expected)");
            screens[static_cast<std::size_t>(l) - 1] = stream;
        }
    }

    // Open the new file if the path changes (the current file is only closed if the new one is opened).
    if (path != nullptr && (initial || std::filesystem::path(path->toString()) != path_))
    {
        std::ofstream file(path->toString(), std::ios::out | std::ios::trunc);
        if (!file)
            mThrowRException("Cannot open log file '", path->toString(), "'");
        file_.close();
        file_ = std::move(file);
        path_ = path->toString();
    }

    // Apply the parameters
    level_.store(level, std::memory_order_relaxed);
    screenLevel_.store(screenLevel, std::memory_order_relaxed);
    screens_ = screens;
}


//------------------------------------------------------------------------------
std::string_view RLogger::toString(const RLogLevel level)
{
    switch(level)
    {
        case RLogLevel::None:    return "NONE";
        case RLogLevel::Error:   return "ERROR";
        case RLogLevel::Warning: return "WARN";
        case RLogLevel::Info:    return "INFO";
        case RLogLevel::Debug:   return "DEBUG";
    }
    return "UNKNOWN";
}


//------------------------------------------------------------------------------
RLogLevel RLogger::toLevel(const std::string_view name)
{
    std::string lower(name);
    std::transform(lower.begin(), lower.end(), lower.begin(), [](const unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    if (lower == "none")
        return RLogLevel::None;
    if (lower == "error")
        return RLogLevel::Error;
    if (lower == "warning")
        return RLogLevel::Warning;
    if (lower == "info")
        return RLogLevel::Info;
    if (lower == "debug")
        return RLogLevel::Debug;
    mThrowRException("Invalid log level '", name, "'");
}


//------------------------------------------------------------------------------
void RLogger::write(const RLogLevel level, const std::string_view module, const std::string_view file, const int line, const std::string_view message)
{
    if (!isEnabled(level))
        return;

    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif

    // Keep only the file name (__FILE__ may contain the full path).
    const auto pos = file.find_last_of("/\\");
    const std::string_view name = pos == std::string_view::npos ? file : file.substr(pos + 1);

    // Build the entry
    std::ostringstream entry;
    entry << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << '.'
          << std::right << std::setfill('0') << std::setw(3) << ms.count() << std::setfill(' ')
          << ' ' << std::left << std::setw(5) << toString(level)
          << ' ' << std::setw(ModuleWidth) << module.substr(0, ModuleWidth)
          << ' ' << message << " (" << name << ':' << line << ')' << '\n';
    const auto str = entry.str();

    const std::lock_guard lock(mutex_);
    if (level <= this->level())
        file_ << str << std::flush;
    if (level <= screenLevel())
        *screens_[static_cast<std::size_t>(level) - 1] << str << std::flush;
}




//------------------------------------------------------------------------------
//
// class RLogFunction
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
RLogBlock::RLogBlock(RLogger& logger, const std::string_view module, const std::string_view file, const int line, std::string name):
    logger_(logger), module_(module), file_(file), line_(line), name_(std::move(name)),
    active_(logger.isEnabled(RLogLevel::Debug))
{
    if (active_)
        logger_.write(RLogLevel::Debug, module_, file_, line_, name_ + " ->");
}


//------------------------------------------------------------------------------
RLogBlock::~RLogBlock()
{
    // A destructor must not throw.
    if (!active_)
        return;
    try
    {
        logger_.write(RLogLevel::Debug, module_, file_, line_, "<- " + name_);
    }
    catch(...)
    {
    }
}
