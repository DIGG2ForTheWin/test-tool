#ifndef TEST_CONFIG_H
#define TEST_CONFIG_H

#include <string>
#include <variant>
#include <filesystem>
#include <cstdint>
#include <optional>
#include "run_config.h"

struct TestCase {
    std::string name;
    std::string from;
    std::string to;
    std::optional<int> count;           //1 if not defined in test.yaml
    std::variant<int, std::string> payload; //std::variant<std::uint64_t, std::filesystem::path> payload;
};

struct MessageHeader {
    std::string party_id_type;
    std::string sender_role;
    std::string receiver_role;
    std::string service;
    std::string service_type;
    std::string action;
};

struct TestConfig {
    std::filesystem::path this_pmode;
    std::filesystem::path other_pmode;
    MessageHeader message_header;
    std::vector<TestCase> tests;
    std::filesystem::path source;
};


TestConfig parse_test_config(const std::filesystem::path& file, const RunConfig& run_config);


#endif