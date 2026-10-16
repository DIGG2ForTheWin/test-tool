#ifndef RUN_H
#define RUN_H

#include <string>

std::string make_run_id();
std::string message_name(std::string_view test, std::string_view run_id, const std::uint64_t n, const std::uint64_t count);


#endif