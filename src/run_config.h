#ifndef RUN_CONFIG_H
#define RUN_CONFIG_H

#include <string>
#include <filesystem>
#include <vector>
#include "node_config.h"
#include "cli.h"
#include "test_config.h"

struct RunPath {
    std::filesystem::path plugin_root;
    std::filesystem::path staging;
    std::filesystem::path fs_out;
};

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

    RunPath paths;
};

RunConfig resolve(const NodeConfig& file, const Options& cli, const TestConfig& test_file);

RunPath build_file_path(const std::filesystem::path& data_root);

void check_avalible_storage(const std::filesystem::path plugin_root, const std::vector<TestCase>& tests);

#endif