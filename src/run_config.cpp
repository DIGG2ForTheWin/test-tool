#include "run_config.h"
#include "errors.h"
#include "test_config.h"
#include "size.h"
#include <filesystem>
#include <set>
#include <string_view>
#include <system_error>
#include <variant>
#include <algorithm>

namespace {
    namespace fs = std::filesystem;
    constexpr std::string_view testfile_dir = "testfiles";
    constexpr std::string_view fs_plugin_dir = "fs_plugin_data";

    std::string node_alternatives (const std::set<std::string>& nodes) {
        std::string alt = "";
        for (const auto& node: nodes) {
           
            if (!alt.empty()) alt += ", ";

            alt += node;
        }

        return alt;
    }

    void check_directory(const fs::path& dir) {
        if (!fs::exists(dir)) {
            throw ConfigError("'" + fs::absolute(dir).string() + "' doesn't exist");
        }

        if (!fs::is_directory(dir)) {
            throw ConfigError("'" + fs::absolute(dir).string() + "' is not a directory");
        }
    }
}

RunConfig resolve(const NodeConfig& node_file, const Options& cli, const TestConfig& test_file) {
    RunConfig run_config;

    run_config.node = cli.node.value_or(node_file.node);
    run_config.data_root = cli.data_root.value_or(node_file.data_root);

    if (node_file.nodes.size() != 2) {
        throw ConfigError("'nodes' in " + node_file.source.filename().string() + " must contain exactly two nodes (found " 
            + std::to_string(node_file.nodes.size()) + ")");
    }

    if (!node_file.nodes.contains(run_config.node)) {
        std::string available_nodes = node_alternatives(node_file.nodes);

        if (cli.node) {
            throw ConfigError("unknown node from --node '" + *cli.node + "'; " 
            + node_file.source.filename().string() + " defines: " + available_nodes);
        }

        auto config_name = node_file.source.filename().string();
        throw ConfigError("unknown node from " + config_name + " '" + run_config.node + "'; " 
            + config_name + " defines: " + available_nodes);
    }

    for (const auto& node: node_file.nodes) {
        if (node != run_config.node) run_config.other_node = node;
    }    
    
    if (!fs::is_directory(run_config.data_root)) {
        throw ConfigError("'" + fs::absolute(run_config.data_root).string() + "' is not a directory");
    }

    for (const auto& test: test_file.tests) {
        if (test.from != run_config.node && test.from != run_config.other_node) {
            throw ConfigError(test_file.source.filename().string() + ": 'tester.tests[" + test.name 
                + "].from' is not a node in " + node_file.source.filename().string() + ", got '"
                + test.from + "' (expected '" + run_config.node + "', '" + run_config.other_node + "')");
        }

        if (test.to != run_config.node && test.to != run_config.other_node) {
            throw ConfigError(test_file.source.filename().string() + ": 'tester.tests[" + test.name 
                + "].to' is not a node in " + node_file.source.filename().string() + ", got '"
                + test.to + "' (expected '" + run_config.node + "', '" + run_config.other_node + "')");
        }
    }

    const bool run_all = cli.tests.size() == 1 && cli.tests[0] == "all";

    if (run_all) {
        for (const auto& test_case: test_file.tests) {
            if (test_case.from == run_config.node) {
                run_config.tests.push_back(test_case);
            }
        }

        if (run_config.tests.empty()) {
            throw ConfigError("no tests in " + test_file.source.filename().string() + " sending from '" + run_config.node + "'");
        }

    } else {
        std::set<std::string> seen_names;
        for (const auto& test_name: cli.tests) {
            auto it = std::ranges::find(test_file.tests, test_name, &TestCase::name);

            if (it == test_file.tests.end()) {
                throw UsageError("unknown test '" + test_name + "'");
            }

            if (it->from != run_config.node) {
                throw ConfigError("test '" + it->name + "' sends from '" + it->from 
                    + "', but this machine is '" + run_config.node + "'");
            }

            if(!seen_names.insert(it->name).second) continue;
            
            run_config.tests.push_back(*it);
        }
    }

    for (auto& test_case: run_config.tests) {
        auto* existing = std::get_if<ExistingFile>(&test_case.payload);
        if (!existing) continue;

        const fs::path full_path = run_config.data_root / testfile_dir / existing->path;

        if (!fs::exists(full_path)) {
            throw ConfigError(test_file.source.filename().string() + ": 'tester.tests[" 
                + test_case.name + "].file' not found: '" + full_path.string() + "'");
        }

        if (!fs::is_regular_file(full_path)) {
            throw ConfigError(test_file.source.filename().string() + ": 'tester.tests[" 
                + test_case.name + "].file' isn't a file: '" + full_path.string() + "'");
        }

        existing->path = full_path;
    }

    run_config.message_header = test_file.message_header;

    return run_config;
}

RunPath build_file_path(const fs::path& data_root) {
    RunPath paths;

    paths.plugin_root = data_root / fs_plugin_dir;
    check_directory(paths.plugin_root);
    
    paths.fs_out = paths.plugin_root / "OUT";
    check_directory(paths.fs_out);

    paths.staging = paths.plugin_root / ".staging";
    std::error_code ec;

    fs::create_directory(paths.staging, ec);

    if (ec) {
        throw RunError("could not create directory '" + fs::absolute(paths.staging).string() + "': " + ec.message());
    }

    return paths;
}

void check_available_storage(const std::filesystem::path& plugin_root, const std::vector<TestCase>& tests) {
    std::uint64_t size_needed = 0;

    for (const auto& test : tests) {
        if (auto* generate = std::get_if<GeneratedFile>(&test.payload)) {
            size_needed += (generate->size_bytes * test.count);
        }
    }

    size_needed += size_needed / 10; //adds ~10%

    std::error_code ec;
    const auto available = fs::space(plugin_root, ec).available;

    if (ec) throw RunError("'" + fs::absolute(plugin_root).string() + "' couldn't retrieve available space: " + ec.message());

    if (available < size_needed) {
        throw RunError("need " + format_size(size_needed) + ", only " + format_size(available) + " available on '" + fs::absolute(plugin_root).string() + "'");
    }

}