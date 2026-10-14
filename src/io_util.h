#ifndef IO_UTIL_H
#define IO_UTIL_H

#include <fstream>
#include <filesystem>
#include <string>

[[noreturn]] void fail_and_remove(std::ofstream& out, const std::filesystem::path& path, const std::string& message);

#endif