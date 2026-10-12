#ifndef YAML_UTIL_H
#define YAML_UTIL_H

#include <optional>
#include <string>
#include <yaml-cpp/yaml.h>
#include <filesystem>

YAML::Node read_file(const std::filesystem::path& file, const std::string& tips = "");

std::string get_value(const YAML::Node& parent ,const std::string& key, const std::string& prefix = "");                            //required - throws if missing
std::optional<std::string> get_optional_value(const YAML::Node& parent ,const std::string& key, const std::string& prefix = "");    //may be absent

YAML::Node get_map(const YAML::Node& parent ,const std::string& key, const std::string& prefix = "");

YAML::Node get_sequence(const YAML::Node& parent ,const std::string& key, const std::string& prefix = "");

#endif