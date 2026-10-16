#ifndef TEST_CONFIG_H
#define TEST_CONFIG_H

#include <string>
#include <variant>
#include <filesystem>
#include <cstdint>
#include <vector>
#include <map>

struct GeneratedFile { std::uint64_t size_bytes; };
struct ExistingFile { 
    // After parsing: relative to <data_root>/testfiles.
    // After resolve(): absolute path to the file.
    std::filesystem::path path; 
};

struct TestCase {
    std::string name;
    std::string from;
    std::string to;
    std::uint64_t count = 1;           
    std::variant<GeneratedFile, ExistingFile> payload; 
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
    std::map<std::string, std::filesystem::path> pmodes;
    MessageHeader message_header;
    std::vector<TestCase> tests;
    std::filesystem::path source;
};


TestConfig parse_test_config(const std::filesystem::path& file);


#endif