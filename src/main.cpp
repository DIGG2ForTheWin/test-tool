#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <yaml-cpp/yaml.h>
#include "cli.h"



int main(int argc, char* argv[]) {
    const std::string error_prefix = "tester: error: ";

    std::vector<std::string> args;

    for(int i = 1; i < argc; ++i) {
        args.push_back(argv[i]);
    }

    try {
        Options opt = parse_options(args);

        std::cout << "Node: " << opt.node << "\nPath to data: " << opt.data_path << "\nTests:";
        for (const auto &test: opt.tests) {
            std::cout << "\n" << test;
        }

    }
    catch(const UsageError& e) {
        std::cerr << error_prefix << e.what() << '\n';
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