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
#include "utilities.hpp"
#include "config.hpp"


auto pivotRoot(const std::string& newroot, const std::string& oldroot) -> void
{
  int ret = syscall(SYS_pivot_root, newroot.c_str(), oldroot.c_str());
  if (ret == -1) {
    panicOnError("pivot_root");
  }
}


auto writeTo(std::filesystem::path path, const std::string& line, std::ios::openmode mode) -> void
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


auto readAll(int fd) -> std::string
{
  unsigned length = 0;

  if (read(fd, &length, sizeof(length)) != sizeof(length)) {
    panicOnError("read_len");
  }

  length = ntohl(length);
  std::vector<char> buf(length);
  unsigned total = 0;

  while (total < length) {
    ssize_t ret = read(fd, buf.data() + total, length - total);
    if (ret <= 0) panicOnError("read_data");
    total += ret;
  }

  return std::string{buf.begin(), buf.end()};
}


auto writeAll(int fd, const std::string& str) -> void
{
  unsigned total = 0;
  unsigned len = str.size();
  unsigned length = htonl(str.size());
  const char* data = str.data();

  if (write(fd, reinterpret_cast<char*>(&length), sizeof(length)) != sizeof(length)) {
    panicOnError("write_len");
  }

  while (total < len) {
    ssize_t n = write(fd, data + total, len - total);
    if (n <= 0) panicOnError("write_data");
    total += n;
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
