#include <array>
#include <cstdint>
#include <cstring>
#include <csignal>
#include <iostream>
#include <memory>
#include <string>
#include <new>
#include <vector>
#include <fstream>
#include <linux/sched.h>
#include <sched.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <poll.h>
#include <sys/mount.h>
#include <sys/capability.h>

auto raiseAndDie(std::string s) -> void
{
  perror(s.c_str());
  std::terminate();
}


auto createNamespace(auto childF, void* args) -> int
{
  std::vector<std::byte> stack(64 * 1024);
  void* sp = stack.data() + stack.size();
  int pid = clone(childF, sp, SIGCHLD | CLONE_NEWNS | CLONE_NEWPID | CLONE_NEWUSER | CLONE_NEWUTS, args);
  if (pid < 0) {
    perror("clone");
    _exit(1);
  }

  return pid;
}


auto writeAll(int fd, std::string&& str)
{
  unsigned total = 0;
  unsigned len = str.size();
  unsigned length = htonl(str.size());
  char* data = str.data();

  if (write(fd, reinterpret_cast<char*>(&length), sizeof(length)) != sizeof(length)) {
    raiseAndDie("write");
  }

  while (total < len) {
    ssize_t n = write(fd, data + total, len - total);
    if (n <= 0) raiseAndDie("write");
    total += n;
  }
}


auto readAll(int fd) -> std::string
{
  unsigned length = 0;

  if (read(fd, &length, sizeof(length)) != sizeof(length)) {
    raiseAndDie("read");
  }

  length = ntohl(length);
  std::vector<char> buf(length);
  unsigned total = 0;

  while (total < length) {
    ssize_t ret = read(fd, buf.data() + total, length - total);
    if (ret <= 0) raiseAndDie("read");
    total += ret;
  }

  return std::string{buf.begin(), buf.end()};
}


auto writeTo(const std::string& path, const std::string& line, const std::ios::openmode mode) -> void
{
  std::ofstream file(path, mode);
  file << line;
}


auto setHostname(const std::string& s) -> void
{
  if (sethostname(s.c_str(), s.size()) == -1) {
    raiseAndDie("sethostname");
  }
}


auto main() -> int
{
  // sv[0] is parent and sv[1] is for the child.
  std::unique_ptr<int[]> sv = std::make_unique<int[]>(2);

  if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv.get()) != 0) {
    raiseAndDie("socketpair");
  }

  int parentfd = sv[0];
  int childfd = sv[1];

  struct SData {
    std::unique_ptr<int[]>& fd;
  };

  SData data { sv };

  int cpid = createNamespace(+[](void* fd) -> int {
    SData& data = *reinterpret_cast<SData*>(fd);
    int parentfd = data.fd[0];
    int childfd = data.fd[1];
    pid_t pid = getpid();

    close(parentfd);

    setHostname("hins");

    // when MS_REMOUNT is passed, the first and third argument are ignored.
    mount("proc", "/proc", "proc", 0, nullptr);
    mount("/", "/", "ext4", MS_REMOUNT | MS_BIND | MS_RDONLY, nullptr);


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
    exit(1);
  },static_cast<void*>(&data));

  close(childfd);

  writeTo(
          "/proc/" + std::to_string(cpid) + "/uid_map",
          std::string("1000 ") + std::to_string(getuid()) + std::string(" 1"),
          std::ios::trunc
          );


  writeTo(
          "/proc/" + std::to_string(cpid) + "/gid_map",
          std::string("1000 ") + std::to_string(getuid()) + std::string(" 1"),
          std::ios::trunc
          );

  bool sentComm = 0;
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
        writeAll(parentfd, "/bin/bash");
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
