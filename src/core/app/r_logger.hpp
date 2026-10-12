/*

    EvoComb

    RLogger.hpp

    Logger - Header.

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
#include <array>
#include <atomic>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <string_view>


//------------------------------------------------------------------------------
namespace evocomb {
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
 * The RLogLevel provides the different log levels. They are ordered from the least detailed (None) to the most detailed
 * (Debug).
 */
enum class RLogLevel
{
    None,               ///< Nothing is logged (only used as a limit).
    Error,              ///< Error.
    Warning,            ///< Warning.
    Info,               ///< Information.
    Debug               ///< Debug information (only if EVOCOMB_DEBUG_LOG is defined).
};


//------------------------------------------------------------------------------
/**
 * The RLogger instance provides a way to write into a log.
 *
 * Each entry is written on one line in a text file:
 * @code
 * 2026-09-27 15:42:07.123 ERROR r.core       Cannot open file (r_io_file.cpp:252)
 * 2026-09-27 15:42:07.125 WARN  r.gga        Group is empty (r_gga_groups.hpp:187)
 * @endcode
 *
 * The module name is aligned on ModuleWidth characters (longer names are truncated), and the source file and line are
 * written at the end.
 *
 * The entries can also be displayed on the screen, with their own level. For each level, the entries are displayed
 * either on std::cerr or on std::cout.
 *
 * The logger is configured with a JSON string:
 * @code
 * {
 *   "path": "app.log",          // Log file (mandatory).
 *   "level": "info",            // Most detailed level written in the file (mandatory).
 *   "screen": {                 // Screen display (optional).
 *     "level": "error",         // Most detailed level displayed (default: "error").
 *     "error": "cerr",          // Output of the errors (default: "cerr").
 *     "warning": "cerr",        // Output of the warnings (default: "cerr").
 *     "info": "cout",           // Output of the information (default: "cout").
 *     "debug": "cout"           // Output of the debug information (default: "cout").
 *   }
 * }
 * @endcode
 * The levels are "none", "error", "warning", "info" or "debug" and the outputs "cerr" or "cout". By default, only the
 * errors are displayed on the screen (on std::cerr). Use "none" as screen level to display nothing.
 *
 * The configuration can be changed with updateConfig (only the parameters present are modified). If the path changes,
 * the current file is closed and the new one is opened (and emptied).
 *
 * The file is emptied when the logger is created. Each entry is flushed immediately, so that the log is complete even
 * if the program crashes. The method write is thread-safe.
 *
 * In practice, the macros mLogError, mLogWarn, mLogInfo and mLogDebug should be used. They accept any number of values
 * (strings, numbers or any type with an operator<<), which are concatenated. Real numbers are written with the shortest
 * representation that gives back exactly the same value (0.1 is written "0.1", 1.0/3 is written "0.3333333333333333").
 * Use evocomb::RConcat::fixed to write a real number with a fixed number of decimals:
 * @code
 * mLogModule("r.core");
 *
 * void f(evocomb::RLogger& log, const std::string& name, int size)
 * {
 *     mLogInfo(log, "Reading '", name, "' (", size, " bytes)");
 * }
 * @endcode
 */
class RLogger
{
public:

    /**
     * Width reserved for the module name in the log.
     */
    static constexpr std::size_t ModuleWidth = 12;

    /**
     * Construct the logger. The file is created if necessary, otherwise its previous content is erased.
     * @param json               Configuration (see the class description).
     * @exception RException     The configuration is invalid or the file cannot be opened.
     */
    explicit RLogger(const std::string& json);

    /**
     * Update the configuration of the logger. The JSON string has the same format as the configuration passed to the
     * constructor, but all the parameters are optional: only those present are modified:
     * @code
     * log.updateConfig(R"({"level": "debug", "screen": {"level": "none"}})");
     * @endcode
     * If the path is different from the current one, the current file is closed and the new one is opened (and
     * emptied). The update is atomic: if a parameter is invalid or the new file cannot be opened, nothing is modified.
     * @param json               JSON string.
     * @exception RException     The configuration is invalid or the file cannot be opened.
     */
    void updateConfig(const std::string& json);

    /**
     * Get the path of the log file.
     * @return a std::filesystem::path.
     */
    [[nodiscard]] std::filesystem::path path() const;

    /**
     * Get the most detailed level written in the file.
     * @return a RLogLevel value.
     */
    [[nodiscard]] RLogLevel level() const { return level_.load(std::memory_order_relaxed); }

    /**
     * Get the most detailed level displayed on the screen.
     * @return a RLogLevel value.
     */
    [[nodiscard]] RLogLevel screenLevel() const { return screenLevel_.load(std::memory_order_relaxed); }

    /**
     * Look if an entry of a given level is written in the file or displayed on the screen.
     * @param level              Level of the entry.
     * @return `true` if this is the case, `false` otherwise.
     */
    [[nodiscard]] bool isEnabled(const RLogLevel level) const { return level != RLogLevel::None && (level <= this->level() || level <= screenLevel()); }

    /**
     * Write an entry in the file and/or on the screen (if its level is enabled).
     * @param level              Level of the entry.
     * @param module             Name of the module.
     * @param file               Name of the source file (only its file name is written).
     * @param line               Line in the source file.
     * @param message            Message.
     */
    void write(RLogLevel level, std::string_view module, std::string_view file, int line, std::string_view message);

    /**
     * Get the string representation of a level.
     * @param level              Level.
     */
    [[nodiscard]] static std::string_view toString(RLogLevel level);

    /**
     * Get a level from its name ("none", "error", "warning", "info" or "debug", the case is ignored).
     * @param name               Name.
     * @exception RException     The name is invalid.
     */
    [[nodiscard]] static RLogLevel toLevel(std::string_view name);

    //-----------------------------------------------------
    RLogger(const RLogger &) = delete;
    RLogger &operator=(const RLogger &) = delete;

private:

    /**
     * Apply a configuration (see the constructor and updateConfig).
     * @param json               JSON string.
     * @param initial            Is it the initial configuration (the path and the level are then mandatory)?
     * @exception RException     The configuration is invalid or the file cannot be opened.
     */
    void applyConfig(const std::string& json, bool initial);

    //-----------------------------------------------------
    std::atomic<RLogLevel> level_{RLogLevel::Info};                                         ///< Most detailed level written in the file.
    std::atomic<RLogLevel> screenLevel_{RLogLevel::Error};                                  ///< Most detailed level displayed on the screen.
    std::array<std::ostream*, 4> screens_{&std::cerr, &std::cerr, &std::cout, &std::cout};     ///< Screen output for each level (Error to Debug).
    std::filesystem::path path_;                                                               ///< Path of the log file.
    std::ofstream file_;                                                                       ///< Log file.
    mutable std::mutex mutex_;                                                                 ///< Mutex protecting the writing and the configuration.
};


//------------------------------------------------------------------------------
/**
 * The RLogBlock class logs the entry and the exit of a a code block: its constructor writes "name ->" and its
 * destructor writes "<- name" (at the level RLogLevel::Debug). It should be used through the mLogBlock macro:
 * @code
 * void f(evocomb::RLogger& log)
 * {
 *     mLogBlock(log, "f");
 *     ...
 * }   // "<- f" is written here, even if an exception is thrown.
 * @endcode
 */
class RLogBlock
{
public:
    /**
     * Construct the object and log the entry in the function.
     * @param logger             Logger.
     * @param module             Name of the module.
     * @param file               Name of the source file.
     * @param line               Line in the source file.
     * @param name               Name of the function.
     */
    RLogBlock(RLogger& logger, std::string_view module, std::string_view file, int line, std::string name);

    /**
     * Log the exit of the function.
     */
    ~RLogBlock();

    //-----------------------------------------------------
    RLogBlock(const RLogBlock&) = delete;
    RLogBlock& operator=(const RLogBlock&) = delete;

private:

    //-----------------------------------------------------
    RLogger& logger_;                       ///< Logger.
    std::string_view module_;               ///< Name of the module.
    std::string_view file_;                 ///< Name of the source file.
    int line_;                              ///< Line in the source file.
    std::string name_;                      ///< Name of the function.
    bool active_;                           ///< Is the entry logged?
};


}  //------- End of namespace evocomb ----------------------------------------------


//------------------------------------------------------------------------------
/**
 * Define the module name used by the mLog* macros (and by the macros throwing exceptions). It must be used once, at
 * file scope, in each source file that calls these macros:
 * @code
 * mLogModule("r.core");
 * @endcode
 * In a header, it must be used inside each function calling these macros (a definition at file scope would conflict
 * with the one of another header).
 *
 * The modules of EvoComb are "r.core" (core), "r.ga", "r.gga", "r.nngga", "r.sgga", "r.hga", "r.2dga" and "r.promethee"
 * (optimization), "r.unit_tests" (tests), and "r.bp", "r.hierarchy" and "r.vlsi" (examples).
 */
#define mLogModule(name) static constexpr const char* evocomb_log_module_name = name


//------------------------------------------------------------------------------
/*
 * The mLogError, mLogWarn, mLogInfo and mLogDebug macros take a logger and any number of values that are concatenated
 * into the message:
 * @code
 * LOG_ERROR(log, "Cannot read line ", line, " of '", file, "'");
 * @endcode
 * The values are only evaluated if the level is enabled in the logger (see RLogger::isEnabled).
 */
#define mLogWrite(logger, level, ...) \
    do { \
        if ((logger).isEnabled(level)) \
            (logger).write(level, evocomb_log_module_name, __FILE__, __LINE__, evocomb::RConcat::concat(__VA_ARGS__)); \
    } while (false)


//------------------------------------------------------------------------------
#define mLogError(logger, ...) mLogWrite(logger, evocomb::RLogLevel::Error, __VA_ARGS__)
#define mLogWarn(logger, ...) mLogWrite(logger, evocomb::RLogLevel::Warning, __VA_ARGS__)
#define mLogInfo(logger, ...) mLogWrite(logger, evocomb::RLogLevel::Info, __VA_ARGS__)


//------------------------------------------------------------------------------
#ifdef EVOCOMB_DEBUG_LOG
    #define mLogDebug(logger, ...) mLogWrite(logger, evocomb::RLogLevel::Debug, __VA_ARGS__)
#else
    // Neither logger nor the values are evaluated (sizeof avoids a warning for an unused logger).
    #define mLogDebug(logger, ...) static_cast<void>(sizeof(logger))
#endif


//------------------------------------------------------------------------------
// Build a unique variable name for LOG_FUNCTION.
#define mLogConcat_(a, b) a##b
#define mLogConcat(a, b) mLogConcat_(a, b)


//------------------------------------------------------------------------------
/**
 * Log "name ->" now and "<- name" when the current scope (typically the function) is left. Like LOG_DEBUG, it is
 * ignored if EVOCOMB_DEBUG_LOG is not defined or if the debug level is not enabled in the logger.
 */
#ifdef EVOCOMB_DEBUG_LOG
    #define mLogBlock(logger, name) \
        const evocomb::RLogBlock mLogConcat(evocomb_log_function_, __LINE__)((logger), evocomb_log_module_name, __FILE__, __LINE__, (name))
#else
    #define mLogBlock(logger, name) static_cast<void>(sizeof(logger))
#endif
