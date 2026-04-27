/*

    EvoComb - Bin Packing example

    main.cpp

    Bin Packing example - Main program.

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
#include "r_bp_gga.hpp"
#include "tools/r_exception.hpp"
#include "tools/r_json.hpp"
using namespace evocomb;
using namespace evocomb::bp;


//------------------------------------------------------------------------------
mLogModule("r.bp");


//------------------------------------------------------------------------------
// Default configuration
static const char* defaultConfig = R"({
    "log": {"path": "bin_packing.log", "level": "info"},
    "population_size": 32,
    "max_generations": 100,
    "ga": {
        "nb_threads": 1,
        "mutation_frequency": 5,
        "strong_mutation_frequency": 10,
        "inversion_frequency": 10,
        "handle_duplicates": true
    },
    "gga": {"heuristic": "first_fit"}
})";


//------------------------------------------------------------------------------
/**
 * Listener logging the progress of the GA.
 */
class Progress : public RGAListener
{
public:
    Progress(RLogger& logger, const RPopulationBP& population) : logger_(logger), population_(population) {}

    void onBest(const RGAInst& inst) override
    {
        mLogInfo(logger_, "Generation ", inst.gen(), ": ", population_[inst.bestInPop()].nbUsed(), " bins");
    }

private:
    RLogger& logger_;
    const RPopulationBP& population_;
};


//------------------------------------------------------------------------------
/**
 * Print the usage.
 * @param name               Name of the program.
 */
static void usage(const char* name)
{
    std::cerr << "Usage: " << name << " <problem> [-c <config.json>] [-o <result.json>]" << std::endl
              << std::endl
              << "Solve a bin packing problem with the Grouping Genetic Algorithm (GGA)." << std::endl
              << std::endl
              << "  <problem>           Problem in JSON ('.json' file, or '-' to read it from the" << std::endl
              << "                      standard input):" << std::endl
              << "                        {\"capacity\": 150, \"objects\": [42, 69, ...], \"min_bins\": 3," << std::endl
              << "                         \"config\": {...}}      (\"min_bins\" and \"config\" are optional)" << std::endl
              << "                      or generated:" << std::endl
              << "                        {\"generate\": {\"nb_bins\": 30, \"capacity\": 150, \"seed\": 42}}" << std::endl
              << "                      A text file (other extensions) contains the number of objects," << std::endl
              << "                      their sizes, the capacity and, optionally, the minimal number" << std::endl
              << "                      of bins." << std::endl
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
/**
 * Look if a path designates a JSON problem ('-' or a '.json' extension).
 * @param path               Path.
 */
static bool isJSON(const std::string& path)
{
    const std::string ext = ".json";
    return (path == "-") || ((path.size() > ext.size()) && (path.compare(path.size() - ext.size(), ext.size(), ext) == 0));
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
        RBPProblem problem;
        auto config = RJSON::parse(defaultConfig);
        if (isJSON(problemPath))
        {
            const auto json = RJSON::parse(readFile(problemPath));
            problem.read(json);
            if (const auto cfg = json.find("config"); cfg != nullptr)
                config.merge(*cfg);
        }
        else
            problem.load(problemPath);
        if (!configPath.empty())
            config.merge(RJSON::parse(readFile(configPath)));
        RLogger logger(config["log"].dump());
        const auto popSize = positive(config, "population_size");
        const auto maxGen = positive(config, "max_generations");
        mLogInfo(logger, problem.objs().size(), " objects, capacity ", problem.capacity(), ", lower bound ", problem.lowerBound(), " bins");

        // Run the GGA
        RPopulationBP population(popSize, RContextBP(problem), config);
        Progress progress(logger, population);
        RGAInst inst(logger, config, &progress);
        inst.init(&population);
        inst.run(maxGen);

        // Result
        const auto& best = population.best();
        RJSON result;
        result.add("nb_objects", std::make_unique<RJSON>(static_cast<long long>(problem.objs().size())));
        result.add("capacity", std::make_unique<RJSON>(static_cast<long long>(problem.capacity())));
        result.add("lower_bound", std::make_unique<RJSON>(static_cast<long long>(problem.lowerBound())));
        if (problem.minBins())
            result.add("min_bins", std::make_unique<RJSON>(static_cast<long long>(problem.minBins())));
        result.add("nb_generations", std::make_unique<RJSON>(static_cast<long long>(inst.gen())));
        result.add("nb_bins", std::make_unique<RJSON>(static_cast<long long>(best.nbUsed())));
        result.add("fitness", std::make_unique<RJSON>(best.fitness()));
        result.add("bins", best.toJSON());
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
