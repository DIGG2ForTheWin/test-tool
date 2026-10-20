#include "node_config.h"
#include <yaml-cpp/yaml.h>
#include <cstddef>
#include <string>
#include <set>
#include "errors.h"
#include "yaml_util.h"

namespace {
    namespace fs = std::filesystem;
}

NodeConfig parse_node_config(const fs::path& file) {
    NodeConfig config;

    try {
        const YAML::Node root = read_file(file, "(use node.example.yaml to create node.yaml)");

        config.source = file;

        if (!root.IsMap()) {
            throw ConfigError("'" + fs::absolute(file).string() + "' is empty or not a YAML map");
        }

        config.node = get_value(root, "node");
        config.data_root = get_value(root, "data_root");

        const YAML::Node nodes = get_sequence(root, "nodes");
        
        for (std::size_t index = 0; index < nodes.size(); ++index) {
            const std::string node = get_element_value(nodes, index, "nodes");

            if(!config.nodes.insert(node).second) {
                throw ConfigError("'nodes[" + std::to_string(index) + "]' repeats '" + node + "'");
            }

        } 
    }
    catch(const ConfigError& c) {
        throw ConfigError(file.filename().string() + ": " + c.what());
    }

    return config;
}