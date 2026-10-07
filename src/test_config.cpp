#include "test_config.h"
#include "yaml_util.h"
#include "errors.h"

//includes for testing/development
#include <iostream>

namespace {
    namespace fs = std::filesystem;

}


TestConfig parse_test_config(const fs::path& file, const RunConfig& run_config) {
    if (!fs::exists(file)) {
        throw ConfigError("'" + fs::absolute(file).string() + "' dosen't exist");
    }

    if (!fs::is_regular_file(file)) {
        throw ConfigError("'" + fs::absolute(file).string() + "' isn't a file");
    }

    TestConfig config;
    config.source = file;

    try {
        const YAML::Node root = read_file(file);

        if (!root.IsMap()) {
            throw ConfigError("'" + fs::absolute(file).string() + "' is empty or not a YAML map");
        }

        const auto this_pmode = run_config.node + "_pmode";
        const auto other_pmode = run_config.other_node + "_pmode";

        

    }
    catch(const ConfigError& c)
    {
        throw ConfigError(file.filename().string() + ": " + c.what());;
    }
    
    return config;
}