#include <sys/mount.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <errno.h>
#include <mqueue.h>
#include "error_handling.hpp"
#include "config.hpp"

auto cleanup() -> void
{
  if (umount2((ROOTFS_PATH / "dev").c_str(), MNT_DETACH) == -1) {
    std::cerr << "umount_rootfs_dev" << ": " << std::strerror(errno) << '\n';
  }

  if (umount2((ROOTFS_PATH / "proc").c_str(), MNT_DETACH) == -1) {
    std::cerr << "umount_rootfs_proc" << ": " << std::strerror(errno) << '\n';
  }

  if (umount2((ROOTFS_PATH / "sys").c_str(), MNT_DETACH) == -1) {
    std::cerr << "umount_rootfs_sys" << ": " << std::strerror(errno) << '\n';
  }

  if (umount2((ROOTFS_PATH / "tmp").c_str(), MNT_DETACH) == 1) {
    std::cerr << "umount_rootfs_tmp" << ": " << std::strerror(errno) << '\n';
  }
}


auto panicOnError(const std::string& msg) -> void
{
  std::cerr << msg << ": " << std::strerror(errno) << std::endl;
  cleanup();
  std::exit(EXIT_FAILURE);
}
