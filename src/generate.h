#ifndef GENERATE_H
#define GENERATE_H

#include <cstdint>
#include <filesystem>

struct WriteStats {
    std::uint64_t bytes;
    double seconds;
    double mib_per_second;
};

WriteStats generate_file(const std::filesystem::path& out_path, const std::uint64_t size_bytes);

#endif