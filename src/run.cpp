#include "run.h"
#include "errors.h"
#include <stdexcept>
#include <string>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <random>
#include <string_view>
#include <cstdint>
#include <algorithm>

namespace {
    constexpr std::size_t default_width = 3;
}

std::string make_run_id() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);

    std::tm parts{};
    if (gmtime_r(&t, &parts) == nullptr) {
        throw RunError("could not convert the current time to UTC");
    }

    std::ostringstream ss;
    ss << std::put_time(&parts, "%Y%m%dT%H%M%SZ");

    std::random_device rd;
    std::uniform_int_distribution<unsigned> dist(0, 0xFFFF);
    const unsigned suffix = dist(rd);

    ss << '-' << std::hex << std::setw(4) << std::setfill('0') << suffix;

    return ss.str();
}

std::string message_name(std::string_view test, std::string_view run_id, const std::uint64_t n, const  std::uint64_t count){
    if (n == 0 || n > count ) {
        throw std::logic_error("message number " + std::to_string(n) + " is out of range (1-" + std::to_string(count) + ")");
    }

    const std::size_t width = std::max<std::size_t>(std::to_string(count).size(), default_width);

    std::ostringstream ss;
    ss << test << '_' << run_id << '_' << std::setw(width) << std::setfill('0') << n;
   
    return ss.str();
}
