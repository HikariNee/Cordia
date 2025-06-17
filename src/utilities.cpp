#include <vector>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <csignal>
#include <linux/sched.h>
#include <sys/syscall.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sched.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pwd.h>
#include "utilities.hpp"
#include "config.hpp"


auto pivotRoot(const std::string& newroot, const std::string& oldroot) -> void
{
  int ret = syscall(SYS_pivot_root, newroot.c_str(), oldroot.c_str());
  if (ret == -1) {
    panicOnError("pivot_root");
  }
}


auto writeTo(const std::filesystem::path& path, const std::string& line, std::ios::openmode mode) -> void
{
  std::ofstream file(path, mode);

  if (!file) {
    panicOnError("could not open file.");
  }

  file << line;
}


auto setHostname(const std::string& name) -> void
{
  int ret = sethostname(name.c_str(), name.size());
  if (ret == -1) {
    panicOnError("sethostname");
  }
}


auto ensureStoreDirectory() -> void
{
  if (!std::filesystem::exists(ROOTFS_PATH)) {
    std::filesystem::create_directory(ROOTFS_PATH.parent_path());
    std::filesystem::create_directory(ROOTFS_PATH);

    // ideally this should be transferred to a
    std::filesystem::permissions(ROOTFS_PATH, std::filesystem::perms::owner_all | std::filesystem::perms::group_all, std::filesystem::perm_options::add);
    std::cout << ("Please put the rootfs you want to use in" + std::string(ROOTFS_PATH)) << ".\n";
  }
}


auto messageTypeToString(MessageType msg) -> std::string
{
  return std::to_string(static_cast<unsigned>(msg));
}
