#include "staging.h"
#include "generate.h"
#include "metadata.h"
#include "run.h"
#include "run_config.h"
#include "test_config.h"
#include "errors.h"
#include "path_guard.h"
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <chrono>
#include <system_error>
#include <variant>

namespace {
    namespace fs = std::filesystem;
}

MessageResult stage_message(const RunPath& paths, const MessageHeader& header_info, const TestCase& test, std::string_view run_id, const std::uint64_t n) {
    MessageResult result;
    
    const auto timed = [](auto&& work) {
        const auto start = std::chrono::steady_clock::now();
        work();
        const std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - start;
        return elapsed.count();
    };
    
    result.name = message_name(test.name, run_id, n, test.count);

    const fs::path staging_dir = paths.staging / result.name;

    std::error_code create_ec;
    const bool created =fs::create_directory(staging_dir, create_ec);

    if (create_ec) {
        throw RunError("could not create directory '" + fs::absolute(staging_dir).string() + "': " + create_ec.message());
    } else if (!created) {
        throw RunError("staging directory '" + fs::absolute(staging_dir).string() + "' already exists (run id collision?)");
    }

    PathGuard guard {staging_dir};

    result.metadata_seconds = timed([&]{write_metadata(staging_dir, header_info, test.from, test.to);});
    

    if (const auto* generate = std::get_if<GeneratedFile>(&test.payload)) {
        WriteStats stats{};
        result.payload_seconds = timed([&] {stats =generate_file(staging_dir /(result.name + ".bin"), generate->size_bytes);});
        result.bytes = stats.bytes;
    } else if (const auto* existing = std::get_if<ExistingFile>(&test.payload)) {
        result.hard_linked = true;
        std::error_code link_ec;

        const fs::path hard_link_target = staging_dir / (result.name + existing->path.extension().string());

        result.payload_seconds = timed([&] {fs::create_hard_link(existing->path, hard_link_target , link_ec);});

        if (link_ec == std::errc::cross_device_link) {
            throw RunError("hard link requires '" + fs::absolute(existing->path).string() + "' and '" + fs::absolute(hard_link_target).string() + "' to be on the same filesystem");
        } else if (link_ec) {
            throw RunError("failed to create hard link from '" + fs::absolute(existing->path).string() + "' to '" + fs::absolute(hard_link_target).string() + "': " + link_ec.message());
        }

        std::error_code size_ec;
        result.bytes = fs::file_size(existing->path, size_ec);

        if (size_ec) throw RunError("failed to retrieve file size for '" + fs::absolute(existing->path).string() + "': " + size_ec.message());
    }

    const fs::path target = paths.fs_out / result.name;

    if(fs::exists(target)) throw RunError("'" + fs::absolute(target).string() + "' already exists");

    std::error_code rename_ec;
    result.rename_seconds = timed([&] {fs::rename(staging_dir, target, rename_ec);});

    if (rename_ec == std::errc::cross_device_link) {
        throw RunError("'" + fs::absolute(staging_dir).string() + "' and '" + fs::absolute(target).string() + "' are not on the same filesystem");
    } else if (rename_ec) {
        throw RunError("failed to move '" + fs::absolute(staging_dir).string() + "' to '" + fs::absolute(target).string() + "': " + rename_ec.message());
    }

    guard.commit();

    return result;
}