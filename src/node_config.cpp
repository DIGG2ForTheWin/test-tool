#include "node_config.h"
#include <yaml-cpp/yaml.h>
#include "errors.h"
#include "yaml_util.h"

namespace {
    namespace fs = std::filesystem;
}

NodeConfig parse_node_config(const fs::path& file) {
    NodeConfig config;

    try {
        const YAML::Node root = read_file(file, "(use example.node.yaml to create node.yaml)");

        config.source = file;

        if (!root.IsMap()) {
            throw ConfigError("'" + fs::absolute(file).string() + "' is empty or not a YAML map");
        }

        config.node = get_value(root, "node");
        config.data_root = get_value(root, "data_root");

        const YAML::Node nodes = get_map(root, "nodes");
        
        for (const auto& entry : nodes) {
            std::string key = entry.first.as<std::string>();
            const YAML::Node value = entry.second;

            const std::string prefix = "nodes." + key;

            if (!value.IsMap()) {
                throw ConfigError("'" + prefix + "' is not a map");
            }

            NodeEndpoint n;
            n.url = get_value(value, "url", prefix);
            n.username = get_value(value, "username", prefix);
            n.password = get_value(value, "password", prefix);

            config.nodes[key] = n;
        }
    }
    catch(const ConfigError& c) {
        throw ConfigError(file.filename().string() + ": " + c.what());
    }

    return config;
}