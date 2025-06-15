#pragma once

#include <mqueue.h>
#include <string>
#include "config.hpp"

class MsgQueue {
public:
  MsgQueue(const std::string&);
  void send(MessageType, unsigned);
  void send(const std::string&, unsigned);
  std::string recv();
  void close();
  mqd_t getQueue();
  ~MsgQueue();

private:
  std::string name;
  mqd_t queue;
  long mq_msgsize;
};
