#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <iomanip>
#include <chrono>
#include <algorithm>
#include <cstdint>
#include <string_view>
#include "size.h"
#include "cli.h"
#include "node_config.h"
#include "run_config.h"
#include "errors.h"
#include "staging.h"
#include "test_config.h"
#include "run.h"

namespace {
    namespace fs = std::filesystem;
    constexpr std::string_view error_prefix = "tester: error: ";
    constexpr std::string_view usage = "\nusage: tester run <test...|all> [--node <name>] [--data-root <path>] [--node-config <path>] [--test-config <path>]\n";
    
    enum {
        success = 0,
        misc_error = 1,
        usage_error = 2,
        config_error = 3
    };

    std::size_t message_name_width(const std::vector<TestCase>& tests, std::string_view run_id) {
        std::size_t width = 0;
        for (const auto& test : tests) {
            const std::string longest = message_name(test.name, run_id, test.count, test.count);
            width = std::max(width, longest.size());
        }
        return width;
    }

    void print_message_line(const MessageResult& result, std::size_t name_width) {
        std::cout << std::left
                << std::setw(static_cast<int>(name_width) + 2) << result.name
                << std::setw(10) << format_size(result.bytes)
                << std::fixed << std::setprecision(3)
                << "metadata " << result.metadata_seconds << " s  "
                << "payload "  << result.payload_seconds  << " s  "
                << "rename "   << result.rename_seconds   << " s";

        if (result.hard_linked) {
            std::cout << "  (hard link)";
        }
        std::cout << '\n';
    }

    void print_summary(std::uint64_t messages, std::uint64_t total_bytes, double total_seconds) {
        std::cout << "done: " << messages << (messages == 1 ? " message, " : " messages, ")
                << format_size(total_bytes) << ", "
                << std::fixed << std::setprecision(1) << total_seconds << " s\n";
    }

    std::uint64_t get_total_count(const std::vector<TestCase>& tests) {
        std::uint64_t result = 0;

        for (const auto& test: tests) {
            result += test.count;
        }

        return result;
    }
}


int main(int argc, char* argv[]) {
    std::vector<std::string> args;
    const fs::path default_node_config = "node.yaml";
    const fs::path default_test_config = "tests.yaml";
   

    for(int i = 1; i < argc; ++i) {
        args.push_back(argv[i]);
    }

    try {
        Options cli = parse_options(args);
        NodeConfig node_file = parse_node_config(cli.node_config.value_or(default_node_config));
        TestConfig test_file = parse_test_config(cli.test_config.value_or(default_test_config));
        RunConfig run_config = resolve(node_file, cli, test_file);
        run_config.paths = build_file_path(run_config.data_root);

        check_available_storage(run_config.paths.plugin_root, run_config.tests);

        const std::string run_id = make_run_id();
        std::cout << "run id: " + run_id + "\n\n";

        const std::size_t name_width = message_name_width(run_config.tests, run_id);

        std::uint64_t messages = 0;
        std::uint64_t total_bytes = 0;
        const std::uint64_t total_count = get_total_count(run_config.tests);
        const auto run_start = std::chrono::steady_clock::now();
        
        try {
            for (const auto& test : run_config.tests) {
                for (std::uint64_t n = 1; n <= test.count; ++n) {
                    const MessageResult result = stage_message(run_config.paths, run_config.message_header, test, run_id, n);
                    print_message_line(result, name_width);
                    ++messages;
                    total_bytes += result.bytes;
                }
            }
        } catch (...) {
            if (messages > 0) std::cerr << "tester: " << messages << " of " << total_count << " messages were already placed in OUT before the error" << std::endl;
            throw;
        }
        

        const std::chrono::duration<double> run_elapsed = std::chrono::steady_clock::now() - run_start;
        std::cout << '\n';
        print_summary(messages, total_bytes, run_elapsed.count());

    }
    catch (const UsageError& e) {
        std::cerr << error_prefix << e.what() << "\n";
        std::cerr << usage;
        return usage_error;
    }
    catch (const ConfigError& c) {
        std::cerr << error_prefix << c.what() << "\n";
        return config_error;
    }
    catch (const std::exception& e) {
        std::cerr << error_prefix << e.what() << "\n";
        return misc_error;
    }
    
    
    return success;
}