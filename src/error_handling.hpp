#pragma once

#include <string>

auto panicOnError(const std::string& msg) -> void;
auto cleanup() -> void;
