#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <yaml-cpp/yaml.h>
#include "cli.h"
#include "node_config.h"

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    const std::string error_prefix = "tester: error: ";
    std::vector<std::string> args;
    fs::path cfg = "node.yaml";
   

    for(int i = 1; i < argc; ++i) {
        args.push_back(argv[i]);
    }

    try {
        Options opt = parse_options(args);
        NodeConfig nc = parse_node_config(cfg);

    }
    catch (const ConfigError& c) {
        std::cerr << error_prefix << c.what() << "\n";
    }
    catch (const UsageError& e) {
        std::cerr << error_prefix << e.what() << "\n";
        std::cerr << "usage: tester run <test...|all> --node <red|blue> [--data-root <path>]\n";
        return 2;
    }

    /* try
    {
        YAML::Node test = YAML::LoadFile("cpp_test.yaml");

        std::string phrase = test["wow"].as<std::string>();

        std::cout<<phrase<<std::endl;

    }
    catch(const YAML::Exception& e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    } */
    
    
    return 0;
}