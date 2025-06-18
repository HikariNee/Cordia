#include <iostream>
#include <cstring>
#include "utilities.hpp"
#include "mq.hpp"

MsgQueue::MsgQueue(const std::string& name) : name(name), queue(0), mq_msgsize(0)
{
  const mqd_t tqueue = mq_open(name.c_str(), O_RDWR | O_CREAT, S_IRWXU, nullptr);
  struct mq_attr mqa {};

  if (tqueue == ((mqd_t) - 1))
    panicOnError("mq_open");

  if (mq_getattr(tqueue, &mqa) == -1) {
    panicOnError("mq_getattr");
  }

  this->queue = tqueue;
  this->mq_msgsize = mqa.mq_msgsize;
}


auto MsgQueue::send(MessageType msg, unsigned prio) -> void
{
  MsgQueue::send(messageTypeToString(msg), prio);
}


auto MsgQueue::send(const std::string& buf, unsigned prio) -> void
{
  int res = mq_send(queue, buf.c_str(), buf.size(), prio);
  if (res == -1)
    panicOnError("mq_send");
}


auto MsgQueue::recv() -> std::string
{
  char msg_ptr[this->mq_msgsize] {};
  int res = mq_receive(queue, msg_ptr, mq_msgsize, NULL);
  if (res == -1)
    panicOnError("mq_recv");
  return std::string{msg_ptr};
}


auto MsgQueue::getQueue() -> mqd_t
{
  return queue;
}


MsgQueue::~MsgQueue()
{
  if (mq_close(queue) == -1) {
    std::cerr << "close_queue" << ": " << std::strerror(errno) << '\n';
  }

  if (mq_unlink(this->name.c_str()) == -1) {
    std::cerr << "unlink_queue" << ": " << std::strerror(errno) << '\n';
  }
}
