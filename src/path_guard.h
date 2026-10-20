#ifndef PATH_GUARD_H
#define PATH_GUARD_H

#include <filesystem>

// Removes a file or directory (recursively) when the guard goes out of scope,
// unless commit() has been called. Used to clean up half-written output
// when something throws.
class PathGuard {
public:
    [[nodiscard]] explicit PathGuard(std::filesystem::path path);
    ~PathGuard();

    void commit() noexcept;

    PathGuard(const PathGuard&) = delete;
    PathGuard& operator=(const PathGuard&) = delete;

private:
    std::filesystem::path path_;
    bool committed_ = false;
};

#endif