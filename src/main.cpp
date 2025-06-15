#include <csignal>
#include <cstdlib>
#include <vector>
#include <iostream>
#include <memory>
#include <string>
#include <filesystem>
#include <format>
#include <thread>
#include <chrono>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <mqueue.h>
#include <unistd.h>
#include <sys/wait.h>
#include "utilities.hpp"
#include "container.hpp"
#include "mount_rootfs.hpp"
#include "config.hpp"
#include "error_handling.cpp"
#include "mq.hpp"

auto main() -> int
{
  ensureStoreDirectory();

  MsgQueue queue {"/msgqu"};
  Container container {};

  // make sure /proc, /dev, and /sys are created properly.
  rootFS::preMountRootFS(ROOTFS_PATH);

  container.initializeContainerWith(+[](void* data) -> int {
    MsgQueue queue { "/msgqu" };
    pid_t pid = getpid();

    rootFS::mountRootFS(ROOTFS_PATH);
    queue.send(MessageType::ROOTFS_READY, 32767); // On Linux, sysconf(_SC_MQ_PRIO_MAX) returns 32768.
    execlp("/bin/sh", "/bin/sh", nullptr);

    std::cout << "killed.\n";
    std::exit(0);
  });

  bool sentComm = 0;

  MessageType msg = static_cast<MessageType>(std::stoi(queue.recv()));

  if (msg == MessageType::ROOTFS_READY) {
    container.setupMaps("0 0 1", "0 0 1");
  }

  waitpid(container.getChildPID(), nullptr, 0);
  return 0;
}
