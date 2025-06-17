#pragma once

#include <memory>
#include <any>
#include <filesystem>
#include <string>
#include <csignal>
#include <vector>
#include <filesystem>
#include <linux/sched.h>
#include <sys/syscall.h>
#include <sys/mount.h>
#include <sched.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <mqueue.h>
#include "config.hpp"
#include "error_handling.hpp"

auto writeTo(const std::filesystem::path&, const std::string&, const std::ios::openmode) -> void;
auto pivotRoot(const std::string&, const std::string&) -> void;
auto setHostname(const std::string&) -> void;
auto ensureStoreDirectory() -> void;
auto messageTypeToString(MessageType) -> std::string;
