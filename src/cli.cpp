#include "cli.h"
#include <algorithm>
#include <iostream>

Options parse_options(const std::vector<std::string> &args) {
    std::vector<std::string> positional;
    Options opt;
    std::size_t pos = 0;
    
    auto next_arg = [&args, &pos] () -> std::string {
        if ((pos + 1) >= args.size()) {
            throw UsageError("option '" + args.at(pos) + "' requires a value");
        }

        if (args.at(pos + 1).starts_with("--")) {
            throw UsageError("option '" + args.at(pos) + "' requires a value, got '" 
            + args.at(pos + 1) + "'");
        }

        return args[++pos];
    };

    while(pos < args.size()) {
        const auto &argument = args[pos];

        if (!argument.starts_with("--")) {
            positional.push_back(args.at(pos));
            ++pos;
            continue;
        }

        if (argument == "--node") {
            const auto next = next_arg();

            if (next != "blue" && next != "red") {
                throw UsageError("invalid value for '--node': '" + next + "' (expected 'red' or 'blue')");
            }

            opt.node = next;

        } else if (argument == "--data-root") {
            opt.data_path = next_arg();

        } else {
            throw UsageError("unknown option: '" + args.at(pos) + "'");
        } 

        ++pos; 
    }

    if (opt.node.empty()) {
        throw UsageError("no node given; use --node <red|blue>"); //(or set TESTER_NODE)när miljövariabler finns
    }

    std::vector<std::string> tests;

    if (positional.empty()) {
        throw UsageError("missing command (expected 'run')");
    }

    if (positional.at(0) != "run") {
        throw UsageError("unknown command: '" + positional.at(0) + "' (expected 'run')");
    }
    
    tests = std::vector<std::string>(positional.begin() + 1, positional.end());

    if (tests.empty()) {
        throw UsageError("'run' needs at least one test name, or 'all'");
    }

    if(std::find(tests.begin(), tests.end(), "all") != tests.end() && tests.size() > 1) {
        throw UsageError("'all' cannot be combined with other test names");

    }
    
    opt.tests = tests;
    
    return opt;
}
