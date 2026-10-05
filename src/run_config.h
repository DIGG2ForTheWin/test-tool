#ifndef RUN_CONFIG_H
#define RUN_CONFIG_H

#include <string>
#include <filesystem>
#include <vector>
#include "node_config.h"
#include "cli.h"

struct RunConfig {
    std::string node;
    std::string other_node;
    std::filesystem::path data_root;

    NodeEndpoint this_endpoint;
    NodeEndpoint other_endpoint;

    std::vector<std::string> tests;
};

RunConfig resolve(const NodeConfig& file, const Options& cli);

#endif