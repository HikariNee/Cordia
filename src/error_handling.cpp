#include <sys/mount.h>
#include <cstdlib>
#include <errno.h>
#include "error_handling.hpp"
#include "config.hpp"

auto cleanup() -> void
{
  if (umount2((ROOTFS_PATH / "dev").c_str(), MNT_DETACH) == -1) {
    perror("umount_rootfs_dev");
  }

  if (umount2((ROOTFS_PATH / "proc").c_str(), MNT_DETACH) == -1) {
    perror("umount_rootfs_proc");
  }

  if (umount2((ROOTFS_PATH / "sys").c_str(), MNT_DETACH) == -1) {
    perror("umount_rootfs_sys");
  }

  if (umount2((ROOTFS_PATH / "tmp").c_str(), MNT_DETACH) == 1) {
    perror("umount_rootfs_tmp");
  }
}


auto panicOnError(const std::string& msg) -> void
{
  perror(msg.c_str());
  cleanup();
  std::exit(EXIT_FAILURE);
}
