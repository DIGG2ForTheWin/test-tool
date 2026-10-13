#include "size.h"
#include "errors.h"
#include <map>
#include <string>
#include <cstdint>

namespace {
    const std::map<std::string, std::uint64_t> units {{"B", 1ULL},
                                                {"KiB", 1ULL << 10},
                                                {"MiB", 1ULL << 20},
                                                {"GiB", 1ULL << 30}};

    const std::map<std::string, std::string> unit_tips = {{"KB", "KiB"},
                                                        {"kB", "KiB"},
                                                        {"Kb", "KiB"},
                                                        {"kb", "KiB"},
                                                        {"MB", "MiB"},
                                                        {"mB", "MiB"},
                                                        {"Mb", "MiB"},
                                                        {"mb", "MiB"}, 
                                                        {"GB", "GiB"},
                                                        {"gB", "GiB"},
                                                        {"Gb", "GiB"},
                                                        {"gb", "GiB"}};

}

std::uint64_t parse_unsigned(const std::string& text, const std::string& full_key) {
    bool only_digits = std::ranges::all_of(text, [](unsigned char c) {
        return std::isdigit(c) != 0;
    });

    if (!only_digits) {
        throw ConfigError("'" + full_key + "' must be a whole number, got '" + text + "'");
    }

    try {
        return static_cast<std::uint64_t>(std::stoull(text));
    } catch (const std::out_of_range&) {
        throw ConfigError("'" + full_key + "' is too large, got '" + text + "'");
    }
}

std::uint64_t parse_size(const std::string& text, const std::string& full_key) {
    const std::size_t pos = text.find_first_not_of("0123456789");
    
    if (pos == std::string::npos) {
        throw ConfigError("'" + full_key + "' is missing a unit (B, KiB, MiB, GiB), got '" + text + "'");
    }

    if (pos == 0) {
        throw ConfigError("'" + full_key + "' must start with a number, got '" + text + "'");
    }

    std::string text_unit = text.substr(pos);

    if(text_unit.starts_with(' ')) text_unit = text_unit.substr(1);

    if (text_unit.empty()) {
        throw ConfigError("'" + full_key + "' is missing a unit (B, KiB, MiB, GiB), got '" + text + "'");
    }

    auto it = units.find(text_unit);

    if (it == units.end()) {
        const std::string all = "B, KiB, MiB or GiB";
        const auto unit_it = unit_tips.find(text_unit);

        const std::string tip = (unit_it != unit_tips.end()) ? "(use " + unit_it->second + ")" 
                                                                : "(use " + all + ")";

        throw ConfigError("'" + full_key + "' has unknown unit '" + text_unit + "' " + tip);
    }

    const auto multiplier = it->second;
    const auto value = parse_unsigned(text.substr(0,pos), full_key);
    const auto max_in_unit = std::numeric_limits<std::uint64_t>::max() / multiplier;

    if (value > max_in_unit) {
        throw ConfigError("'" + full_key + "' is too large, got '" + text + "' (max is " 
            + std::to_string(max_in_unit) + " " + text_unit + ")");
    }

    if (value == 0) {
        throw ConfigError("'" + full_key + "' must be greater than 0, got '" + text + "'");
    }

    return value * multiplier;
}