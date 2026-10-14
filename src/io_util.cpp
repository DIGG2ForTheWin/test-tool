#include "io_util.h"
#include "errors.h"
#include <system_error>

namespace  {
    namespace fs = std::filesystem;
}

[[noreturn]] void fail_and_remove(std::ofstream& out, const std::filesystem::path& path, const std::string& message) {
    out.close();
    std::error_code ec;

    fs::remove(path, ec);

    if (ec) {
        throw RunError(message + " (could not remove partial file: " + ec.message() + ")");
    } 

    throw RunError(message);
}