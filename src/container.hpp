#pragma once
#include <memory>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>
#include <sched.h>
#include "mq.hpp"

class Container {
public:
  Container() = default;

  // guess why I defined this again...
  template<typename F>
  auto initializeContainerWith(F func) -> void
  {
    int cpid = createNamespace(func, nullptr);
    this->cpid = cpid;
  }

  void setupMaps(const std::string&, const std::string&);
  int getChildPID();
  ~Container();

private:
  int cpid;

  template<typename T>
  auto createNamespace(T childF, void* args) -> int
  {
    // setup an initial stack, we arent going to use this anyway after we start up some program.
    std::vector<std::byte> stack(64 * 1024);
    void* sp = stack.data() + stack.size();

    // CLONE_NEWNS -> new mount namespace
    int pid = clone(childF, sp, SIGCHLD | CLONE_NEWNS | CLONE_NEWPID | CLONE_NEWUSER | CLONE_NEWUTS, args);
    if (pid < 0) {
      panicOnError("clone");
    }

    return pid;
  }
};
