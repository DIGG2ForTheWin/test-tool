#include "run_config.h"

namespace {
    namespace fs = std::filesystem;

    void check_url (const NodeEndpoint& endpoint, const std::string& which_node ) {
        if (endpoint.url.ends_with("/")) {
            throw ConfigError("'nodes." + which_node + ".url' has ending slash (/)");
        }
        if (!endpoint.url.starts_with("http://") && !endpoint.url.starts_with("https://")) {
            throw ConfigError("'nodes." + which_node + ".url' must start with 'http://' or 'https://' ("
            + endpoint.url + ")");
        }
    }

    std::string node_alternatives (const std::map<std::string, NodeEndpoint>& node_config) {
        std::string alt = "";
        for (const auto& [key, value]: node_config) {
            std::string temp_alt;

            if (alt.empty()) {
                temp_alt = key;
            } else {
                temp_alt = ", " + key;
            }

            alt += temp_alt;
        }

        return alt;
    }
}

RunConfig resolve(const NodeConfig& file, const Options& cli) {
    RunConfig run_config;

    run_config.node = cli.node.value_or(file.node);
    run_config.data_root = cli.data_root.value_or(file.data_root);
    run_config.tests = cli.tests; 

    if (file.nodes.size() != 2) {
        throw ConfigError("'nodes' in node.yaml must contain exactly two nodes (found " 
            + std::to_string(file.nodes.size()) + ")");
    }

    if (!file.nodes.contains(run_config.node)) {
        std::string available_nodes = node_alternatives(file.nodes);

        if (cli.node) {
            throw ConfigError("unknown node from --node '" + *cli.node + "'; " 
            + file.source.filename().string() + " defines: " + available_nodes);
        }
        auto config_name = file.source.filename().string();
        throw ConfigError("unknown node from " + config_name + " '" + run_config.node + "'; " 
            + config_name + " defines: " + available_nodes);
    }

    run_config.this_endpoint = file.nodes.at(run_config.node);

    for (const auto& [key, value]: file.nodes) {
        if (key != run_config.node) {
            run_config.other_node = key;
            run_config.other_endpoint = value;
        }
    }
    
    if (!fs::is_directory(run_config.data_root)) {
        throw ConfigError("'" + fs::absolute(run_config.data_root).string() + "' is not a directory");
    }

    check_url(run_config.this_endpoint, run_config.node);
    check_url(run_config.other_endpoint, run_config.other_node);

    return run_config;
}