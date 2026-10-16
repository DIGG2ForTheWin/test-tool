#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include "cli.h"
#include "node_config.h"
#include "run_config.h"
#include "errors.h"
#include "test_config.h"
#include "metadata.h"
#include "run.h"

namespace {
    namespace fs = std::filesystem;
    constexpr std::string_view error_prefix = "tester: error: ";
    constexpr std::string_view usage = "\nusage: tester run <test...|all> [--node <name>] [--data-root <path>] [--node-config <path>] [--test-config <path>]";
    
    enum {
        success = 0,
        misc_error = 1,
        usage_error = 2,
        config_error = 3
    };

    void print_run_config(const RunConfig& rc) {
        std::cout << "\n\nthis node: " << rc.node << " | other node: " << rc.other_node
            << "\n\nthis node config:\n"
            << " - url: " << rc.this_endpoint.url
            << "\n - username: " << rc.this_endpoint.username << "\n\n"
            << "other node config:\n"
            << " - url: " << rc.other_endpoint.url
            << "\n - username: " << rc.other_endpoint.username << "\n\n"
            << "data-root: " << rc.data_root << "\n\n"
            << "--- PModes ---\n"
            << "- this_pmode: " << rc.this_pmode
            << "\n - other_pmode: " << rc.other_pmode
            << "\n\n--- Tests ---\n\n";

            for (std::size_t i = 0; i < rc.tests.size(); ++i) {
                const auto& test = rc.tests[i];

                std::cout << "------\n|nr." << i + 1 << "|"
                << "\nname: " << test.name
                << "\nfrom: " << test.from
                << "\nto: " << test.to
                << "\ncount: " << test.count;

                const auto* size_existing = std::get_if<GeneratedFile>(&test.payload);
                const auto* file_existing = std::get_if<ExistingFile>(&test.payload);

                if (size_existing) std::cout << "\nsize: " << size_existing->size_bytes;
                if (file_existing) std::cout << "\nfile: " << file_existing->path;

                std::cout << "\n------\n";
            }

            std::cout << std::endl;
    }
}


int main(int argc, char* argv[]) {
    std::vector<std::string> args;
    fs::path default_node_config = "node.yaml";
    fs::path default_test_config = "tests.yaml";
   

    for(int i = 1; i < argc; ++i) {
        args.push_back(argv[i]);
    }

    try {
        Options cli = parse_options(args);
        NodeConfig node_file = parse_node_config(cli.node_config.value_or(default_node_config));
        TestConfig test_file = parse_test_config(cli.test_config.value_or(default_test_config));
        RunConfig run_config = resolve(node_file, cli, test_file);

        print_run_config(run_config);
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