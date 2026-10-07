#ifndef YAML_UTIL_H
#define YAML_UTIL_H

#include <yaml-cpp/yaml.h>
#include <filesystem>

YAML::Node read_file(const std::filesystem::path& file);
std::string get_value(const YAML::Node& node ,const std::string& key, const std::string& prefix = "");

#endif