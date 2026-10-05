#ifndef ERRORS_H
#define ERRORS_H

#include <stdexcept>

class ConfigError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class UsageError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

#endif