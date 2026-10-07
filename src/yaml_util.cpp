#include "yaml_util.h"
#include "errors.h"

namespace {
    constexpr std::string_view placeholder = "CHANGE_ME";
    namespace fs = std::filesystem;
}

YAML::Node read_file(const fs::path& file) {
        try {
            const YAML::Node root = YAML::LoadFile(file.string());
            return root;
        } 
        catch(const YAML::BadFile& b){
            throw ConfigError("cannot open file: " + std::string(b.what()));
        } 
        catch(const YAML::Exception& e) {
            throw ConfigError("YAML error: " + std::string(e.what()));
        }
    }

std::string get_value(const YAML::Node& node ,const std::string& key, const std::string& prefix) {
        std::string full_key = (prefix.empty()) ? key : prefix + "." + key;
        const YAML::Node value = node[key];

        if (!value.IsDefined()) {
            throw ConfigError("missing key: '" + full_key + "'");
        }
        if (value.IsNull()) {
            throw ConfigError("'" + full_key + "' is empty");
        }
        if (!value.IsScalar()) {
            throw ConfigError("'" + full_key + "' must be a single value, not a list or map");
        }

        const auto value_string = value.as<std::string>();

        if (value_string.empty()) {
            throw ConfigError("'" + full_key + "' is empty");
        }
        if (value_string == placeholder) {
            throw ConfigError("'" + full_key + "' still has the placeholder value from example.node.yaml");
        }

        return value_string;
    }