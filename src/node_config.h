#ifndef NODE_CONFIG_H
#define NODE_CONFIG_H

#include <string>
#include <map>
#include <stdexcept>
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
};

NodeConfig parse_node_config(const std::filesystem::path& file);

class ConfigError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};



#endif