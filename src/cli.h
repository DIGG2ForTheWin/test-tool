#ifndef CLI_H
#define CLI_H

#include <string>
#include <vector>
#include <filesystem>
#include <optional>


struct Options {
    std::optional<std::string> node;
    std::optional<std::filesystem::path> data_root;
    std::optional<std::filesystem::path> node_config;
    std::vector<std::string> tests;
};

Options parse_options(const std::vector<std::string>& args);

#endif