/*

    EvoComb - VLSI example

    main.cpp

    VLSI example - Main program.

    Copyright 1998-2026 by Pascal Francq (pascal@francq.info).

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/


//------------------------------------------------------------------------------
// include files for ANSI C/C++
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "app/r_logger.hpp"
#include "ga/r_ga_inst.hpp"
#include "ga2d/r_ga2d_population.hpp"
#include "r_vlsi_problem.hpp"
#include "tools/r_exception.hpp"
#include "tools/r_json.hpp"
using namespace evocomb;
using namespace evocomb::vlsi;


//------------------------------------------------------------------------------
mLogModule("r.vlsi");


//------------------------------------------------------------------------------
// Default configuration
static const char* defaultConfig = R"({
    "log": {"path": "vlsi.log", "level": "info"},
    "population_size": 16,
    "max_generations": 30,
    "ga": {
        "maximize": false,
        "nb_threads": 1,
        "mutation_frequency": 5,
        "handle_duplicates": true
    },
    "ga2d": {"heuristic": "bottom_left", "all_orientations": false}
})";


//------------------------------------------------------------------------------
/**
 * Listener logging the progress of the GA.
 */
class Progress : public RGAListener
{
public:
    Progress(RLogger& logger, const RGA2DPopulation<>& population) : logger_(logger), population_(population) {}

    void onBest(const RGAInst& inst) override
    {
        const auto& best = population_[inst.bestInPop()];
        mLogInfo(logger_, "Generation ", inst.gen(), ": area ratio ", best.areaRatio(), ", length ", best.length());
    }

private:
    RLogger& logger_;
    const RGA2DPopulation<>& population_;
};


//------------------------------------------------------------------------------
/**
 * Print the usage.
 * @param name               Name of the program.
 */
static void usage(const char* name)
{
    std::cerr << "Usage: " << name << " <problem.json> [-c <config.json>] [-o <result.json>]" << std::endl
              << std::endl
              << "Place the instances of a VLSI circuit with the 2D placement genetic algorithm." << std::endl
              << std::endl
              << "  <problem.json>      Problem ('-' to read it from the standard input) with the" << std::endl
              << "                      elements \"size\", \"terminals\", \"masters\", \"instances\"," << std::endl
              << "                      \"nets\" and, optionally, \"config\"." << std::endl
              << "  -c <config.json>    Configuration merged with the default one and with the" << std::endl
              << "                      \"config\" object of the problem." << std::endl
              << "  -o <result.json>    Result file (default: the standard output)." << std::endl
              << std::endl
              << "Default configuration:" << std::endl
              << defaultConfig << std::endl;
}


//------------------------------------------------------------------------------
/**
 * Read a text file (or the standard input if the path is "-").
 * @param path               Path of the file.
 */
static std::string readFile(const std::string& path)
{
    if (path == "-")
        return std::string(std::istreambuf_iterator<char>(std::cin), std::istreambuf_iterator<char>());
    std::ifstream file(path);
    if (!file)
        throw RException("Cannot open '" + path + "'");
    std::stringstream str;
    str << file.rdbuf();
    return str.str();
}


//------------------------------------------------------------------------------
/**
 * Get a strictly positive integer parameter of the configuration.
 * @param config             Configuration.
 * @param key                Key of the parameter.
 */
static std::size_t positive(const RJSON& config, const std::string& key)
{
    const auto& param = config[key];
    if ((!param.isInteger()) || (param.toInteger() <= 0))
        throw RException("'" + key + "' must be a strictly positive integer");
    return static_cast<std::size_t>(param.toInteger());
}


//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    // Arguments
    std::string problemPath;
    std::string configPath;
    std::string outputPath;
    for (int i = 1; i < argc; ++i)
    {
        const std::string arg(argv[i]);
        if (((arg == "-c") || (arg == "-o")) && (i + 1 < argc))
            ((arg == "-c") ? configPath : outputPath) = argv[++i];
        else if ((arg == "-h") || (arg == "--help"))
        {
            usage(argv[0]);
            return EXIT_SUCCESS;
        }
        else if (problemPath.empty() && ((arg == "-") || (arg[0] != '-')))
            problemPath = arg;
        else
        {
            usage(argv[0]);
            return EXIT_FAILURE;
        }
    }
    if (problemPath.empty())
    {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    try
    {
        // Problem and configuration
        const auto json = RJSON::parse(readFile(problemPath));
        RVLSIProblem problem;
        problem.read(json);
        auto config = RJSON::parse(defaultConfig);
        if (const auto cfg = json.find("config"); cfg != nullptr)
            config.merge(*cfg);
        if (!configPath.empty())
            config.merge(RJSON::parse(readFile(configPath)));
        RLogger logger(config["log"].dump());
        const auto popSize = positive(config, "population_size");
        const auto maxGen = positive(config, "max_generations");
        const auto& p = problem.problem();
        mLogInfo(logger, p.nbObjs(), " instances, ", p.nets().size(), " nets, circuit ", p.width(), "x", p.height());

        // Run the GA
        RGA2DPopulation<> population(popSize, RGA2DContext(p), config);
        Progress progress(logger, population);
        RGAInst inst(logger, config, &progress);
        inst.init(&population);
        inst.run(maxGen);

        // Result
        const auto& best = population.best();
        const auto bound = best.boundary();
        RJSON result;
        auto size = std::make_unique<RJSON>(RJSONType::Array);
        size->add(std::make_unique<RJSON>(static_cast<long long>(p.width())));
        size->add(std::make_unique<RJSON>(static_cast<long long>(p.height())));
        result.add("size", std::move(size));
        result.add("nb_instances", std::make_unique<RJSON>(static_cast<long long>(p.nbObjs())));
        result.add("nb_nets", std::make_unique<RJSON>(static_cast<long long>(p.nets().size())));
        result.add("nb_generations", std::make_unique<RJSON>(static_cast<long long>(inst.gen())));
        result.add("fitness", std::make_unique<RJSON>(best.fitness()));
        result.add("area_ratio", std::make_unique<RJSON>(best.areaRatio()));
        auto box = std::make_unique<RJSON>(RJSONType::Array);
        for (const auto c : {bound.x1, bound.y1, bound.x2, bound.y2})
            box->add(std::make_unique<RJSON>(static_cast<long long>(c)));
        result.add("bounding_box", std::move(box));
        result.add("length", std::make_unique<RJSON>(best.length()));
        result.add("instances", problem.toJSON(best));
        const auto output = result.dump(2);
        if (outputPath.empty())
            std::cout << output << std::endl;
        else
        {
            std::ofstream file(outputPath);
            if (!file)
                throw RException("Cannot create '" + outputPath + "'");
            file << output << std::endl;
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
