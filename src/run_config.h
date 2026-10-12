#ifndef RUN_CONFIG_H
#define RUN_CONFIG_H

#include <string>
#include <filesystem>
#include <vector>
#include "node_config.h"
#include "cli.h"
#include "test_config.h"

struct RunConfig {
    std::string node;
    std::string other_node;
    std::filesystem::path data_root;

    NodeEndpoint this_endpoint;
    NodeEndpoint other_endpoint;

    std::filesystem::path this_pmode;
    std::filesystem::path other_pmode;

    std::vector<TestCase> tests;

    MessageHeader message_header;
};

RunConfig resolve(const NodeConfig& file, const Options& cli, const TestConfig& test_file);

#endif