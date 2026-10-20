#include "path_guard.h"
#include <filesystem>
#include <iostream>
#include <system_error>
#include <utility>

PathGuard::PathGuard(std::filesystem::path path)
: path_(std::move(path)) {}

PathGuard::~PathGuard() {
    if (!committed_) {
        std::error_code ec;
        std::filesystem::remove_all(path_, ec);

        if (ec) std::cerr << "warning: could not remove '" << path_.string() << "': " << ec.message() << std::endl;
    }
}

void PathGuard::commit() noexcept {
    committed_ = true;
}