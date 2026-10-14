#ifndef GENERATE_H
#define GENERATE_H

#include <cstddef>
#include <cstdint>
#include <filesystem>

constexpr std::size_t default_block_size = 1 << 20;

struct WriteStats {
    std::filesystem::path file;
    std::uint64_t bytes;
    double seconds;
    double mib_per_second;
};

WriteStats generate_file(const std::filesystem::path& out_path, const std::uint64_t size_bytes, const std::size_t block_size = default_block_size);

#endif