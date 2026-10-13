#include "generate.h" 
#include "errors.h"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ios>
#include <random>
#include <string>
#include <vector>

namespace {
    constexpr std::size_t block_size = 1 << 20; // 1 MiB
    namespace fs = std::filesystem;
}

WriteStats generate_file(const std::filesystem::path& out_path, const std::uint64_t size_bytes) {
    std::mt19937_64 gen(std::random_device{}());
    std::vector<char> buffer(block_size);

    std::ranges::generate(buffer, [&gen] {
        return static_cast<char>(gen() & 0xFF);
    });
    
    if (fs::exists(out_path)) {
        throw RunError("'" + fs::absolute(out_path).string() + "' already exists");
    }

    std::ofstream out(out_path, std::ios::binary);

    const auto fail = [&out, &out_path] (const std::string& message) {
        out.close();
        std::error_code ec;

        if (fs::remove(out_path, ec)) {
            throw RunError(message + ": successfull deletion of file");
        } else if (ec) {
            throw RunError(message + ": error occured while deleting file: " + ec.message());
        } else {
            throw RunError(message + ": file didn't exist");
        }
    };

    if (!out) {
        fail("cannot create '" + fs::absolute(out_path).string() + "'");
    }

    std::uint64_t remaining = size_bytes;
    std::uint64_t written_bytes = 0;

    const auto start = std::chrono::steady_clock::now();

    while (remaining > 0) {
        std::uint64_t n = std::min(remaining, static_cast<std::uint64_t>(buffer.size()));

        out.write(buffer.data(), static_cast<std::streamsize>(n));

        if (!out) {
            fail("write failed for '" + fs::absolute(out_path).string() + "' after " + std::to_string(written_bytes / block_size) + 
            " MiB of " + std::to_string(size_bytes / block_size) + " MiB (disk full?)");
        }

        remaining -= n;
        written_bytes += n;
    }

    out.close();

    if (!out) {
        fail("failed to finish writing '" + fs::absolute(out_path).string() + "'");
    }
    const auto end = std::chrono::steady_clock::now();

    const std::chrono::duration<double> elapsed = end - start;
    double seconds = elapsed.count();


    double throughput;
    if (seconds > 0) {
        throughput= written_bytes / seconds / block_size;
    } else {
        throughput = 0;
    }
    

    return {written_bytes, seconds, throughput};
}