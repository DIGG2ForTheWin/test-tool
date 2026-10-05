#ifndef YAML_UTIL_H
#define YAML_UTIL_H

#include <yaml-cpp/yaml.h>

std::string get_value(const YAML::Node& node ,const std::string& key, const std::string& prefix = "");

#endif