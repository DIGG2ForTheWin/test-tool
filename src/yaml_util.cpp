#include "yaml_util.h"
#include "errors.h"
#include <optional>
#include <string_view>

namespace {
    constexpr std::string_view placeholder = "CHANGE_ME";
    namespace fs = std::filesystem;

    std::string scalar_to_string(const YAML::Node& value, const std::string& full_key) {
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
            throw ConfigError("'" + full_key + "' still has the placeholder value '" + std::string(placeholder) + "'");
        }

        return value_string;
    }
}

YAML::Node read_file(const fs::path& file, const std::string& tips) {

    if (!fs::exists(file)) {
        throw ConfigError("'" + fs::absolute(file).string() + "' doesn't exist" + (tips.empty() ? "" : " " + tips));
    }

    if (!fs::is_regular_file(file)) {
        throw ConfigError("'" + fs::absolute(file).string() + "' isn't a file" + (tips.empty() ? "" : " " + tips));
    }

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

std::string get_value(const YAML::Node& parent ,const std::string& key, const std::string& prefix) {
    const std::string full_key = (prefix.empty()) ? key : prefix + "." + key;
    const YAML::Node value = parent[key];

    if (!value.IsDefined()) {
        throw ConfigError("missing key: '" + full_key + "'");
    }

    return scalar_to_string(value, full_key);
}

std::optional<std::string> get_optional_value(const YAML::Node& parent ,const std::string& key, const std::string& prefix) {
    if (!parent[key].IsDefined()) {
        return std::nullopt;
    }

    return get_value(parent, key, prefix);
}

YAML::Node get_map(const YAML::Node& parent ,const std::string& key, const std::string& prefix) {
    std::string full_key = (prefix.empty()) ? key : prefix + "." + key;
    const YAML::Node map = parent[key];

    if (!map.IsDefined()) {
        throw ConfigError("missing key '" + full_key + "'");
    }

    if (map.IsNull()) {
        throw ConfigError("'" + full_key + "' is empty");
    }

    if (!map.IsMap()) {
        throw ConfigError("'" + full_key + "' is not a map");
    }

    if (map.size() == 0) {
        throw ConfigError("'" + full_key + "' is empty");
    }

    return map;
}

YAML::Node get_sequence(const YAML::Node& parent ,const std::string& key, const std::string& prefix ) {
    std::string full_key = (prefix.empty()) ? key : prefix + "." + key;
    const YAML::Node sequence = parent[key];

    if (!sequence.IsDefined()) {
        throw ConfigError("missing key '" + full_key + "'");
    }

    if (sequence.IsNull()) {
        throw ConfigError("'" + full_key + "' is empty");
    }

    if (!sequence.IsSequence()) {
        throw ConfigError("'" + full_key + "' is not a sequence");
    }

    if (sequence.size() == 0) {
        throw ConfigError("'" + full_key + "' is empty");
    }

    return sequence;
}

std::string get_element_value(const YAML::Node& sequence, std::size_t index, const std::string& name) {
    const std::string full_key = name + "[" + std::to_string(index) + "]";
    const YAML::Node value = sequence[index];

    if (!value.IsDefined()) {
        throw ConfigError("missing element: '" + full_key + "'");
    }

    return scalar_to_string(value, full_key);
}