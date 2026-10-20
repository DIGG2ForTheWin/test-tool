#include "test_config.h"
#include "yaml_util.h"
#include "errors.h"
#include "size.h"
#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <map>


namespace {
    namespace fs = std::filesystem;

    std::uint64_t parse_count(const std::string& number, const std::string& full_key) {
       
        const auto count = parse_unsigned(number, full_key);

        if (count == 0) {
            throw ConfigError("'" + full_key + "' can't be less than 1, got '" + std::to_string(count) + "'");
        }

        return count;
    }

    
}


TestConfig parse_test_config(const fs::path& file) {
    

    TestConfig config;
    

    try {
        const YAML::Node root = read_file(file);
        config.source = file;

        const YAML::Node tester = get_map(root, "tester");

        const YAML::Node message = get_map(tester, "message", "tester");

        const std::string message_prefix = "tester.message";

        config.message_header.party_id_type = get_value(message, "party_id_type", message_prefix);
        config.message_header.sender_role = get_value(message, "sender_role", message_prefix);
        config.message_header.receiver_role = get_value(message, "receiver_role", message_prefix);
        config.message_header.service = get_value(message, "service", message_prefix);
        config.message_header.service_type = get_value(message, "service_type", message_prefix);
        config.message_header.action = get_value(message, "action", message_prefix);

        const YAML::Node tests = get_sequence(tester, "tests", "tester");

        std::map<std::string, std::size_t> seen_names;
        for (std::size_t index = 0; index < tests.size(); ++index) {
            const std::string index_prefix = "tester.tests[" + std::to_string(index) + "]";
            const YAML::Node test = tests[index];

            if (!test.IsMap()) {
                throw ConfigError("'" + index_prefix + "' must be a map");
            }

            TestCase test_case;

            test_case.name = get_value(test, "name", index_prefix);

            const auto is_valid_front = [] (unsigned char c) {
                return std::isdigit(c) || std::islower(c);
            };

            if (!is_valid_front(test_case.name.front())) {   // get_value guarantees a non-empty name
                throw ConfigError("'" + index_prefix + ".name' must begin with a letter (a-z) or number (0-9), got '" + test_case.name + "'");
            }
            
            bool valid_char = std::ranges::all_of(test_case.name, [] (unsigned char c) {
                return std::isdigit(c) || std::islower(c) || c == '-' || c == '_';
            });

            if (!valid_char) {
                throw ConfigError("'" + index_prefix + ".name' may only contain a-z, 0-9, '_' and '-', got '" + test_case.name + "'");
            }

            auto [it, inserted] = seen_names.insert({test_case.name, index});
            if (!inserted) {
                throw ConfigError("'" + index_prefix + "' duplicate test name '" + test_case.name + "' (also used by tester.tests[" 
                    + std::to_string(it->second) + "])");
            }

            const std::string prefix = "tester.tests[" + test_case.name + "]";

            test_case.from = get_value(test, "from", prefix);
            test_case.to = get_value(test, "to", prefix);
            

            if (test_case.to == test_case.from) {
                throw ConfigError("'" + prefix + "' 'from' and 'to' must differ, both are '" + test_case.from + "'");
            }

            if (const auto value = get_optional_value(test, "count", prefix); value) {
                test_case.count = parse_count(*value, prefix + ".count");
            }

            std::optional<std::string> size = get_optional_value(test, "size", prefix);
            std::optional<std::string> file_value = get_optional_value(test, "file", prefix);

            if (size && file_value) {
                throw ConfigError("'" + prefix + "' must have either 'size' or 'file', not both");
            }

            if (!size && !file_value) {
                throw ConfigError("'" + prefix + "' must have 'size' or 'file'");
            }

            if (size) {
                const std::string full_key = prefix + ".size";
                test_case.payload = GeneratedFile { parse_size(*size, full_key) };
            }

            if (file_value) {
                const std::string full_key = prefix + ".file";
                const fs::path path = *file_value;

                if (path.is_absolute()) {
                    throw ConfigError("'" + full_key + "' must be relative to <data_root>/testfiles, got '" + path.string() + "'");
                }

                for (const auto& part: path) {
                    if (part == "..") {
                        throw ConfigError("'" + full_key + "' must not contain '..', got '" + path.string() + "'");
                    }
                }
                test_case.payload = ExistingFile { path };
            }

            config.tests.push_back(test_case);
        }

    }
    catch(const ConfigError& c)
    {
        throw ConfigError(file.filename().string() + ": " + c.what());
    }
    
    return config;
}