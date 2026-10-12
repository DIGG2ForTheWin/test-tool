#include "run_config.h"
#include "errors.h"
#include <filesystem>
#include <set>
#include <variant>

namespace {
    namespace fs = std::filesystem;

    void check_url (const NodeEndpoint& endpoint, const std::string& which_node) {
        if (endpoint.url.ends_with("/")) {
            throw ConfigError("'nodes." + which_node + ".url' has ending slash (/)");
        }
        if (!endpoint.url.starts_with("http://") && !endpoint.url.starts_with("https://")) {
            throw ConfigError("'nodes." + which_node + ".url' must start with 'http://' or 'https://' ("
            + endpoint.url + ")");
        }
    }

    std::string node_alternatives (const std::map<std::string, NodeEndpoint>& node_config) {
        std::string alt = "";
        for (const auto& [key, value]: node_config) {
            std::string temp_alt;

            if (alt.empty()) {
                temp_alt = key;
            } else {
                temp_alt = ", " + key;
            }

            alt += temp_alt;
        }

        return alt;
    }
}

RunConfig resolve(const NodeConfig& node_file, const Options& cli, const TestConfig& test_file) {
    RunConfig run_config;

    run_config.node = cli.node.value_or(node_file.node);
    run_config.data_root = cli.data_root.value_or(node_file.data_root);

    if (node_file.nodes.size() != 2) {
        throw ConfigError("'nodes' in node.yaml must contain exactly two nodes (found " 
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

    run_config.this_endpoint = node_file.nodes.at(run_config.node);

    for (const auto& [key, value]: node_file.nodes) {
        if (key != run_config.node) {
            run_config.other_node = key;
            run_config.other_endpoint = value;
        }
    }
    
    if (!fs::is_directory(run_config.data_root)) {
        throw ConfigError("'" + fs::absolute(run_config.data_root).string() + "' is not a directory");
    }

    check_url(run_config.this_endpoint, run_config.node);
    check_url(run_config.other_endpoint, run_config.other_node);

    if (!test_file.pmodes.contains(run_config.node)) {
        throw ConfigError("'tester.pmodes' has no entry for '" + run_config.node + "'");
    }

    if (!test_file.pmodes.contains(run_config.other_node)) {
        throw ConfigError("'tester.pmodes' has no entry for '" + run_config.other_node + "'");
    }

    for (const auto& [key, path]: test_file.pmodes) {
        if(key != run_config.node && key != run_config.other_node) {
            throw ConfigError("'tester.pmodes." + key + "' is not a node in " + node_file.source.filename().string() 
                + " (expected '" + run_config.node + "', '" + run_config.other_node + "')");
        }
    }

    const fs::path this_pmode = test_file.pmodes.at(run_config.node);
    const fs::path other_pmode = test_file.pmodes.at(run_config.other_node);

    if (!fs::is_regular_file(this_pmode)) {
        throw ConfigError(test_file.source.filename().string() + ": PMode for '" + run_config.node 
            + "' not found: '" + this_pmode.string() + "'");
    }

    if (!fs::is_regular_file(other_pmode)) {
        throw ConfigError(test_file.source.filename().string() + ": PMode for '" + run_config.other_node 
            + "' not found: '" + other_pmode.string() + "'");
    }

    run_config.this_pmode = this_pmode;
    run_config.other_pmode = other_pmode;

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

    bool run_all = cli.tests.size() == 1 && cli.tests[0] == "all";

    if (run_all) {
        for (const auto& test_case: test_file.tests) {
            if (test_case.from == run_config.node) {
                run_config.tests.push_back(test_case);
            }
        }

        if (run_config.tests.empty()) {
            throw ConfigError("no tests in " + test_file.source.filename().string() + " send from '" + run_config.node + "'");
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

    for (const auto& test_case: run_config.tests) {
        const auto* existing = std::get_if<ExistingFile>(&test_case.payload);
        if (!existing) continue;

        const fs::path full_path = run_config.data_root / "testfiles" / existing->path;

        if (!fs::exists(full_path)) {
            //tests.yaml: 'tester.tests[dicom_sample].file' not found: '/data/...'
            throw ConfigError(test_file.source.filename().string() + ": 'tester.tests[" 
                + test_case.name + "].file' not found: '" + full_path.string() + "'");
        }

        if (!fs::is_regular_file(full_path)) {
            throw ConfigError(test_file.source.filename().string() + ": 'tester.tests[" 
                + test_case.name + "].file' isn't a file: '" + full_path.string() + "'");
        }
    }

    run_config.message_header = test_file.message_header;

    return run_config;
}