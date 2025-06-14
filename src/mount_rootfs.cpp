#include <sys/mount.h>
#include <sys/stat.h>
#include "utilities.hpp"
#include "mount_rootfs.hpp"
#include "config.hpp"

auto rootFS::preMountRootFS(std::filesystem::path root) -> void
{
  // The sole purpose of the function is to setup the "special" directories inside the new root.
  if (mount("proc", (root / "proc").c_str(), "proc", 0, nullptr) == -1) {
    panicOnError("mount_newrootfs_proc");
  }

  if (mount("dev", (root / "dev").c_str(), "devtmpfs", 0, nullptr) == -1) {
      panicOnError("mount_newrootfs_devtmpfs");
  }

  if (mount("sys", (root / "sys").c_str(), "sysfs", 0, nullptr) == -1) {
    panicOnError("mount_newrootfs_sysfs");
  }

  if (mount("tmp", (root / "tmp").c_str(), "tmpfs", 0, nullptr) == -1) {
    panicOnError("mount_newrootfs_tmp");
  }
}


auto performPivot(std::filesystem::path newroot) -> void
{
  // https://github.com/opencontainers/runc/blob/b04031d708de1ad55df6b7536004500f8e5c3d67/libcontainer/rootfs_linux.go#L1077

  const char* root = newroot.c_str();

  // pivot_root expects newroot to be a mountpoint, the easiest way to make a directory into a mountpoint is to bind it to itself.
  if (mount(root, root, nullptr, MS_BIND | MS_REC, nullptr) == -1) {
    panicOnError("mountrootfs_bind");
  }

  // change to the newroot directory.
  if (chdir(root) == -1) {
    panicOnError("chdir_rootfs");
  }

  // The special magic, pivotroot here swaps the / with the first argument and puts the old / in the second argument
  // As said by the comment in the linked github file, this really shouldn't work but it does.
  // in this case, the oldroot is at /proc/self/cwd.

  pivotRoot(".", ".");

  // change to oldroot, just for safety.
  // switching the current directory to there does not make /proc/self/cwd refer to /proc/self/cwd.
  if (chdir(".") == -1) {
    panicOnError("chdir_oldrootfs");
  }

  // https://www.kernel.org/doc/Documentation/filesystems/sharedsubtree.txt lists the reason for this well enough.
  // by making the oldroot a slave, any changes to it are not propagated back to the host filesystem.
  if (mount("", ".", nullptr, MS_SLAVE | MS_REC, nullptr) == -1) {
    panicOnError("mount_rslave");
  }

  // now unmount the oldroot,  as we have no need for it now really. MNT_DETACH removes it when the device is free.
  if (umount2(".", MNT_DETACH) == -1) {
    panicOnError("umount_cwd");
  }

  // to the actual root!
  if (chdir("/") == -1) {
    panicOnError("chdir_newrootfs");
  }
}

auto rootFS::mountRootFS(std::filesystem::path newroot) -> void
{
  performPivot(newroot);
}
