#ifndef METADATA_H
#define METADATA_H

#include <filesystem>
#include <string>
#include <string_view>

#include "test_config.h"   // MessageHeader

// Replaces & < > " ' with XML entities. Use for every value written into XML.
std::string xml_escape(std::string_view text);

// Builds the complete metadata.xml content for one message (no file I/O).
std::string build_metadata_xml(const MessageHeader& header, std::string_view from, std::string_view to);

// Writes metadata.xml into the given directory. Throws RunError on failure.
void write_metadata(const std::filesystem::path& dir, const MessageHeader& header, std::string_view from, std::string_view to);

#endif