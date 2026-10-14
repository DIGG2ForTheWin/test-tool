#ifndef SIZE_H
#define SIZE_H

#include <string>
#include <cstdint>

std::uint64_t parse_unsigned(const std::string& text, const std::string& full_key);
std::uint64_t parse_size(const std::string& text, const std::string& full_key);
std::string format_size(const std::uint64_t bytes);

#endif