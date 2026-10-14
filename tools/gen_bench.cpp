#include "errors.h"
#include "generate.h"
#include "size.h"

#include <cstddef>
#include <cstdint>
#include <exception>
#include <iomanip>
#include <iostream>
#include <string_view>

namespace {
    constexpr std::string_view error_prefix = "gen_bench: error: ";

    constexpr std::string_view usage =
        "usage:   gen_bench <output-path> <size> [<block-size>]\n"
        "example: gen_bench /tmp/test.bin \"500 MiB\" \"64 KiB\"\n";

    // Guard against accidentally allocating a huge buffer
    constexpr std::uint64_t max_block_size = 256ULL << 20;   // 256 MiB

    enum ExitCode {
        success = 0,
        run_error = 1,
        usage_error = 2,
    };

    // Prints "label:" padded to a fixed width so the values line up
    void print_label(std::string_view label) {
        std::cout << std::left << std::setw(12) << label;
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3 && argc != 4) {
        std::cerr << usage;
        return usage_error;
    }

    try {
        const std::uint64_t size = parse_size(argv[2], "size");

        std::uint64_t block_size = default_block_size;
        if (argc == 4) {
            block_size = parse_size(argv[3], "block-size");
        }

        if (block_size > max_block_size) {
            throw ConfigError("'block-size' must be at most " + format_size(max_block_size)
                              + ", got '" + argv[3] + "'");
        }

        const WriteStats stats =
            generate_file(argv[1], size, static_cast<std::size_t>(block_size));

        print_label("file:");
        std::cout << stats.file.string() << '\n';

        print_label("size:");
        std::cout << format_size(stats.bytes) << '\n';

        print_label("block size:");
        std::cout << format_size(block_size) << '\n';

        print_label("time:");
        std::cout << std::fixed << std::setprecision(3) << stats.seconds << " s\n";

        print_label("throughput:");
        std::cout << std::fixed << std::setprecision(1) << stats.mib_per_second << " MiB/s\n";
    }
    catch (const ConfigError& e) {
        // In this program a ConfigError means a bad command-line argument
        std::cerr << error_prefix << e.what() << '\n' << usage;
        return usage_error;
    }
    catch (const RunError& e) {
        std::cerr << error_prefix << e.what() << '\n';
        return run_error;
    }
    catch (const std::exception& e) {
        std::cerr << error_prefix << e.what() << '\n';
        return run_error;
    }

    return success;
}