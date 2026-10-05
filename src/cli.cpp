#include "cli.h"
#include <algorithm>

Options parse_options(const std::vector<std::string>& args) {
    std::vector<std::string> positional;
    Options opt;
    std::size_t pos = 0;
    
    auto next_arg = [&args, &pos] () -> std::string {
        if ((pos + 1) >= args.size()) {
            throw UsageError("option '" + args.at(pos) + "' requires a value");
        }
        auto& next_arg = args.at(pos + 1);
        if (next_arg.starts_with("--")) {
            throw UsageError("option '" + args.at(pos) + "' requires a value, got '" 
            + args.at(pos + 1) + "'");
        }
        if (next_arg.empty()) {
            throw UsageError("option '" + args.at(pos) + "' requires a non-empty value");
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
            opt.node = next_arg();

        } else if (argument == "--data-root") {
            opt.data_root = next_arg();

        } else if (argument == "--node-config") {
            opt.node_config = next_arg();
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

    if(std::find(opt.tests.begin(), opt.tests.end(), "all") != opt.tests.end() && opt.tests.size() > 1) {
        throw UsageError("'all' cannot be combined with other test names");
    }
    
    return opt;
}
