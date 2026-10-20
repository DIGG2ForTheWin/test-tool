#include "generate.h" 
#include "errors.h"
#include "path_guard.h"
#include "size.h"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ios>
#include <random>
#include <string>
#include <vector>
#include <cerrno>
#include <cstring>

namespace {
    constexpr std::uint64_t bytes_per_mib = 1 << 20;
    namespace fs = std::filesystem;
}

WriteStats generate_file(const std::filesystem::path& out_path, const std::uint64_t size_bytes, const std::size_t block_size) {
    if (fs::exists(out_path)) {
        throw RunError("'" + fs::absolute(out_path).string() + "' already exists");
    }
   
    std::mt19937_64 gen(std::random_device{}());
    std::vector<char> buffer(block_size);

    std::ranges::generate(buffer, [&gen] {
        return static_cast<char>(gen() & 0xFF);
    });

    PathGuard guard {out_path};

    std::ofstream out(out_path, std::ios::binary);

    if (!out) {
        const int err = errno;
        throw RunError("failed to create file '" + fs::absolute(out_path).string() + "': " + std::strerror(err));
    }

    std::uint64_t remaining = size_bytes;
    std::uint64_t written_bytes = 0;

    const auto start = std::chrono::steady_clock::now();

    while (remaining > 0) {
        const std::uint64_t n = std::min(remaining, static_cast<std::uint64_t>(buffer.size()));

        out.write(buffer.data(), static_cast<std::streamsize>(n));

        if (!out) {
            const int err = errno;
            throw RunError("write failed for '" + fs::absolute(out_path).string() + "' after " + format_size(written_bytes) + 
            " of " + format_size(size_bytes) + ": " + std::strerror(err));
        }

        remaining -= n;
        written_bytes += n;
    }

    out.close();

    if (!out) {
        const int err = errno;
        throw RunError("failed to finish writing '" + fs::absolute(out_path).string() + "': " + std::strerror(err));
    }
    const auto end = std::chrono::steady_clock::now();

    guard.commit();

    const std::chrono::duration<double> elapsed = end - start;
    double seconds = elapsed.count();


    double throughput = (seconds > 0) ? written_bytes / seconds / bytes_per_mib : 0;

    return {out_path,written_bytes, seconds, throughput};
}