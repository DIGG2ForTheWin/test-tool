#ifndef CLI_H
#define CLI_H

#include <stdexcept>
#include <string>
#include <vector>


struct Options {
    std::string node;
    std::string data_path = "/data";
    std::vector<std::string> tests;
};

Options parse_options(const std::vector<std::string> &args);

class UsageError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

#endif