#ifndef NODE_CONFIG_H
#define NODE_CONFIG_H

#include <string>
#include <filesystem>
#include <set>



struct NodeConfig {
    std::string node;
    std::filesystem::path data_root;
    std::set<std::string> nodes;
    std::filesystem::path source;
};

NodeConfig parse_node_config(const std::filesystem::path& file);

#endif