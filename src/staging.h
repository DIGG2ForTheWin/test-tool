#ifndef STAGING_H
#define STAGING_H

#include <string>
#include <string_view>
#include <cstdint>
#include "run_config.h"

struct MessageResult {
    std::string name;
    std::uint64_t bytes = 0;
    double metadata_seconds = 0;
    double payload_seconds = 0;
    double rename_seconds = 0;
    bool hard_linked = false;     // true when the payload is a hard link to an existing file
};

MessageResult stage_message(const RunPath& paths, const MessageHeader& header_info, const TestCase& test, std::string_view run_id, const std::uint64_t n);

#endif