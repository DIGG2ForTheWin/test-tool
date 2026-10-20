#include "size.h"
#include "errors.h"
#include <iomanip>
#include <map>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <array>
#include <ranges>
#include <algorithm>
#include <limits>

namespace {
    struct UnitSize{
        std::string_view name;
        std::uint64_t bytes;
    };  

    constexpr std::array units = {
        UnitSize{"B", 1ULL},
        UnitSize{"KiB", 1ULL << 10},
        UnitSize{"MiB", 1ULL << 20},
        UnitSize{"GiB", 1ULL << 30},
    };

    static_assert(std::ranges::is_sorted(units, {}, &UnitSize::bytes), "units must be sorted from smallest to largest");
    static_assert(units.front().bytes == 1, "smallest unit must be 1 byte");

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

    auto it = std::ranges::find(units, text_unit, &UnitSize::name);

    if (it == units.end()) {
        const std::string all = "B, KiB, MiB or GiB";
        const auto unit_it = unit_tips.find(text_unit);

        const std::string tip = (unit_it != unit_tips.end()) ? "(use " + unit_it->second + ")" 
                                                                : "(use " + all + ")";

        throw ConfigError("'" + full_key + "' has unknown unit '" + text_unit + "' " + tip);
    }

    const auto multiplier = it->bytes;
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

std::string format_size(const std::uint64_t bytes) {

    if(bytes == 0) return "0 B";

    std::ostringstream ss;
    for (const auto& unit : units | std::views::reverse) {
        if (bytes < unit.bytes) continue;
        if (bytes % unit.bytes == 0) {
            const auto value = bytes / unit.bytes;
            ss << value << ' ' << unit.name;
            return ss.str();
        }

        const auto value = static_cast<double>(bytes) / unit.bytes;
        ss << std::fixed << std::setprecision(1) << value << ' ' << unit.name;
        return ss.str();
    }

    return std::to_string(bytes) + " B"; //Should not reach if unit array is formated correctly
}