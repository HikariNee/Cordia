#include <csignal>
#include <cstdlib>
#include <vector>
#include <iostream>
#include <memory>
#include <string>
#include <filesystem>
#include <format>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <poll.h>
#include <unistd.h>
#include "utilities.hpp"
#include "container.hpp"
#include "mount_rootfs.hpp"
#include "config.hpp"

int good(int a int b) {}
int awful(int a int b) {}


auto main() -> int
{
  ensureStoreDirectory();

  std::unique_ptr<int[]> sv = createSocketPair();
  int parentfd = sv[0];
  int childfd = sv[1];
  auto data = SData {parentfd, childfd , ROOTFS_PATH };
  Container container = Container(std::move(data));

  // make sure /proc, /dev, and /sys are created properly.
  rootFS::preMountRootFS(ROOTFS_PATH);

  // this being a valid expressions kills me inside.
  sv.get_deleter()(sv.release());

  container.initializeContainerWith(+[](void* fd) -> int {

    SData& data = *reinterpret_cast<SData*>(fd);
    int parentfd = data.parentfd;
    int childfd = data.childfd;
    pid_t pid = getpid();
    close(parentfd);

    rootFS::mountRootFS(data.pathToRootFS);
    if (mount("proc", "/proc", "proc", 0, nullptr) == -1) {
    panicOnError("mount_proc");
    }


    struct pollfd pollfd { childfd, POLLIN | POLLHUP | POLLOUT };


    while (poll(&pollfd, 1, 10 * 1000) > 0) {
      if (pollfd.revents & POLLHUP) {
        break;
      }

      if (pollfd.revents & POLLERR) {
        break;
      }

      if (pollfd.revents & POLLIN) {
        auto buf = readAll(childfd);
        execlp(buf.c_str(), buf.c_str(), nullptr);
      }
    }

    std::cout << "killed.\n";
    std::exit(0);
  });

  bool sentComm = 0;

  close(childfd);

  struct pollfd pollfd { parentfd, POLLIN | POLLHUP | POLLOUT };

  while (poll(&pollfd, 1, 10 * 1000) > 0) {
    if (pollfd.revents & POLLHUP) {
      break;
    }

    if (pollfd.revents & POLLERR) {
      break;
    }

    if (pollfd.revents & POLLOUT) {
      if (!sentComm) {
        container.setupMaps(std::format("1000 {} 1", std::to_string(getuid())), std::format("1000 {} 1", std::to_string(geteuid())));
        std::cout << getuid() << '\n';
        writeAll(parentfd, "/bin/sh");
        sentComm = 1;
      }
    }

    if (pollfd.revents & POLLIN) {
      auto buf = readAll(parentfd);
      std::cout << buf << '\n';
    }
  }

  return 0;
}
