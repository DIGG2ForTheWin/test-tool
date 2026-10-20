#include "cli.h"
#include <algorithm>
#include "errors.h"

Options parse_options(const std::vector<std::string>& args) {
    std::vector<std::string> positional;
    Options opt;
    std::size_t pos = 0;
    
    auto next_arg = [&args, &pos] () -> std::string {
        if ((pos + 1) >= args.size()) {
            throw UsageError("option '" + args.at(pos) + "' requires a value");
        }
        auto& next = args.at(pos + 1);
        if (next.starts_with("--")) {
            throw UsageError("option '" + args.at(pos) + "' requires a value, got '" 
            + next + "'");
        }
        if (next.empty()) {
            throw UsageError("option '" + args.at(pos) + "' requires a non-empty value");
        }

        return args[++pos];
    };

    while(pos < args.size()) {
        const auto &argument = args[pos];

        if (!argument.starts_with("--")) {
            positional.push_back(argument);
            ++pos;
            continue;
        }

        if (argument == "--node") {
            opt.node = next_arg();

        } else if (argument == "--data-root") {
            opt.data_root = next_arg();

        } else if (argument == "--node-config") {
            opt.node_config = next_arg();

        } else if (argument == "--test-config") {
            opt.test_config = next_arg();
            
        } else {
            throw UsageError("unknown option: '" + argument + "'");
        } 

        ++pos; 
    }

    if (positional.empty()) {
        throw UsageError("missing command (expected 'run')");
    }

    if (positional.at(0) != "run") {
        throw UsageError("unknown command: '" + positional.at(0) + "' (expected 'run')");
    }
    
    opt.tests = std::vector<std::string>(positional.begin() + 1, positional.end());

    if (opt.tests.empty()) {
        throw UsageError("'run' needs at least one test name, or 'all'");
    }
    
    if(std::ranges::find(opt.tests, "all") != opt.tests.end() && opt.tests.size() > 1) {
        throw UsageError("'all' cannot be combined with other test names");
    }
    
    return opt;
}
