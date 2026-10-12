/*

    EvoComb - Hierarchy example

    main.cpp

    Hierarchy example - Main program (builds a hierarchy of objects with the
    HGA from a JSON file and writes the result in JSON).

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
#include "r_hierarchy_hga.hpp"
#include "r_hierarchy_problem.hpp"
#include "tools/r_exception.hpp"
#include "tools/r_json.hpp"
using namespace evocomb;
using namespace evocomb::hierarchy;


//------------------------------------------------------------------------------
// Default configuration
static const char* defaultConfig = R"({
    "log": {"path": "hierarchy.log", "level": "warning"},
    "population_size": 16,
    "max_generations": 30,
    "ga": {
        "maximize": false,
        "nb_threads": 1,
        "mutation_frequency": 5,
        "handle_duplicates": true
    },
    "hga": {"heuristic": "first_node"}
})";


//------------------------------------------------------------------------------
/**
 * Print the usage.
 * @param name               Name of the program.
 */
static void usage(const char* name)
{
    std::cerr << "Usage: " << name << " <problem.json> [-c <config.json>] [-o <result.json>]" << std::endl
              << std::endl
              << "Build a hierarchy of objects described by attributes with the Hierarchical" << std::endl
              << "Genetic Algorithm (HGA)." << std::endl
              << std::endl
              << "  <problem.json>      Problem ('-' to read it from the standard input):" << std::endl
              << "                      {\"objects\": [{\"name\": \"...\", \"attributes\": [\"...\", ...]}, ...]," << std::endl
              << "                       \"config\": {...}}                  (\"config\" is optional)" << std::endl
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
 * Get a positive integer parameter of the configuration.
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
        RHierarchyProblem problem;
        problem.read(json);
        auto config = RJSON::parse(defaultConfig);
        if (const auto cfg = json.find("config"); cfg != nullptr)
            config.merge(*cfg);
        if (!configPath.empty())
            config.merge(RJSON::parse(readFile(configPath)));
        RLogger logger(config["log"].dump());
        const auto popSize = positive(config, "population_size");
        const auto maxGen = positive(config, "max_generations");

        // Run the HGA
        RPopulationHierarchy population(popSize, RHGAContext<RHGAObj>(problem.objs()), config);
        RGAInst inst(logger, config);
        inst.init(&population);
        inst.run(maxGen);

        // Result
        const auto& best = population.best();
        RJSON result;
        result.add("nb_objects", std::make_unique<RJSON>(static_cast<long long>(problem.objs().size())));
        result.add("nb_attributes", std::make_unique<RJSON>(static_cast<long long>(problem.nbAttrs())));
        result.add("nb_generations", std::make_unique<RJSON>(static_cast<long long>(inst.gen())));
        result.add("avg_choices", std::make_unique<RJSON>(best.avgChoices()));
        result.add("nb_nodes", std::make_unique<RJSON>(static_cast<long long>(best.nbNodes())));
        result.add("depth", std::make_unique<RJSON>(static_cast<long long>(best.depth())));
        result.add("tree", best.toJSON(problem));
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
