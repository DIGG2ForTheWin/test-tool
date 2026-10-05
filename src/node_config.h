#ifndef NODE_CONFIG_H
#define NODE_CONFIG_H

#include <string>
#include <map>
#include <filesystem>

struct NodeEndpoint {
    std::string url;
    std::string username;
    std::string password;
};

struct NodeConfig {
    std::string node;
    std::filesystem::path data_root;
    std::map<std::string, NodeEndpoint> nodes;
    std::filesystem::path source;
};

NodeConfig parse_node_config(const std::filesystem::path& file);

#endif