#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <utility>
#include <iostream>
#include "utilities.hpp"
#include "mount_rootfs.hpp"
#include "config.hpp"

//dev is treated specially, the OCI spec says that we mustn't mount everything and only mount what is necessary.
// https://github.com/opencontainers/runc/blob/b04031d708de1ad55df6b7536004500f8e5c3d67/libcontainer/SPEC.md?plain=1#L67
auto createDevDirs(const std::filesystem::path& root) -> void
{
  const std::filesystem::path devpath = root / "dev";

  std::filesystem::create_directory(devpath / "pts");
  std::filesystem::create_directory(devpath / "mqueue");
  std::filesystem::create_directory(devpath / "shm");
}


auto mountDevDirs(const std::filesystem::path& root) -> void
{
  const std::filesystem::path devpath = root / "dev";

  if (mount("devpts", (devpath / "pts").c_str(), "devpts", MS_NOEXEC | MS_NOSUID, "mode=620,ptmxmode=0666") == -1) {
    panicOnError("mount_newrootfs_devpts");
  }

  if (mount(nullptr, (devpath / "mqueue").c_str(), "mqueue", MS_NOEXEC | MS_NOSUID | MS_NODEV, nullptr) == -1) {
    panicOnError("mount_newrootfs_devmqueue");
  }

  if (mount(nullptr, (devpath / "shm").c_str(), "tmpfs", MS_NOEXEC | MS_NOSUID | MS_NODEV, "mode=1777,size=65536k") == -1) {
    panicOnError("mount_newrootfs_devshm");
  }
}


auto createBasicDevNodes(const std::filesystem::path& root) -> void
{
  const std::filesystem::path dev = root / "dev";
  // (file, major, minor)
  // https://www.kernel.org/doc/html/latest/admin-guide/devices.html
  std::array<std::tuple<std::string, unsigned, unsigned>, 6> devfiles {
    std::make_tuple("null", 1, 3),
    std::make_tuple("zero", 1, 5),
    std::make_tuple("full", 1, 7),
    std::make_tuple("random", 1, 8),
    std::make_tuple("urandom", 1, 9),
    std::make_tuple("tty", 5, 0),
  };

  for (const auto& [file, major, minor] : devfiles) {
    dev_t device = makedev(major, minor);

    if (mknod((dev / file).c_str(), S_IFCHR | 0666, device) == -1) {
      panicOnError(("/dev/" + file).c_str());
    }
  };
}


auto setupPTMX() -> void
{
  if (symlink("/dev/pts/ptmx", "/dev/ptmx") == -1) {
    panicOnError("symlink_dev_ptmx");
  }
}


auto setupStdFds() -> void
{
  const std::string procself = "/proc/self/fd";
  const std::string dev = "/dev";

  std::array<std::pair<std::string, std::string>, 4> procFdsToDev {
    std::make_pair(procself, dev + "/fd"),
    std::make_pair(procself + "/0" , dev + "/stdin"),
    std::make_pair(procself + "/1" , dev + "/stdout"),
    std::make_pair(procself + "/2" , dev + "/stderr"),
  };

  for (const auto& [proc, devfd] : procFdsToDev) {
    if (symlink(proc.c_str(), devfd.c_str()) == -1) {
      panicOnError("symlink_proc_fd");
    }
  }
}


auto rootFS::preMountRootFS(const std::filesystem::path& root) -> void
{
  const std::filesystem::path devpath = root / "dev";

  // the mount options follow what the OCI spec says.
  if (mount("proc", (root / "proc").c_str(), "proc", MS_NOEXEC | MS_NOSUID | MS_NODEV, nullptr) == -1) {
    panicOnError("mount_newrootfs_proc");
  }

  if (mount("sys", (root / "sys").c_str(), "sysfs", MS_NOEXEC | MS_NOSUID | MS_NODEV | MS_RDONLY, nullptr) == -1) {
    panicOnError("mount_newrootfs_sysfs");
  }

  if (mount("tmp", (root / "tmp").c_str(), "tmpfs", 0, nullptr) == -1) {
    panicOnError("mount_newrootfs_tmp");
  }

  if (mount("dev", (root / "dev").c_str(), "tmpfs", MS_NOEXEC | MS_STRICTATIME, "mode=755") == -1) {
    panicOnError("mount_newrootfs_dev");
  }


  createDevDirs(root);
  mountDevDirs(root);
  createBasicDevNodes(root);
}


auto performPivot(const std::filesystem::path& newroot) -> void
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

auto rootFS::mountRootFS(const std::filesystem::path& newroot) -> void
{
  performPivot(newroot);
  setupPTMX();
  setupStdFds();

  if (mount("/", "/", nullptr, MS_REMOUNT | MS_PRIVATE | MS_BIND | MS_RDONLY, nullptr) == -1) {
    panicOnError("remount_rootfs_ro");
  }
}
