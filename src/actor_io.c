
#include <errno.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <unistd.h>
#include "actor_common.h"
#include "actor_def.h"

#define MAX_EVENTS 2

static void* thread_io_worker(void* arg);
static void process_io(actor_io_t* io, int event);

static int epoll_fd;
pthread_t pid;

int actor_io_add(int fd, actor_io_t* io) {
  struct epoll_event ev;
  ACTOR_ASSERT(io != NULL);
  io->recv_r = io->recv_w = io->send_r = io->send_w = 0;
  //   io->recv_buf = ACTOR_MALLOC(io->recv_buf_len);
  //   ACTOR_ASSERT(io->recv_buf != NULL);
  //   io->send_buf = ACTOR_MALLOC(io->send_buf_len);
  //   ACTOR_ASSERT(io->send_buf != NULL);
  ev.events = EPOLLIN;
  ev.data.ptr = io;
  return epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev);
}

int actor_io_del(int fd) {
  return epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
}

int actor_io_write(int fd, void* ud, int enable) {
  struct epoll_event ev;
  ev.events = EPOLLIN | (enable ? EPOLLOUT : 0);
  ev.data.ptr = ud;
  return epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev);
}

void actor_io_init(void) {
  epoll_fd = epoll_create(0);
  if (epoll_fd < 0) {
    ACTOR_PRINT("epoll create error[%d]\n", errno);
    return;
  }
  // create io thread
  pthread_create(&pid, NULL, thread_io_worker, NULL);
  return;
}

void actor_io_deinit(void) {
  pthread_cancel(pid);
  if (epoll_fd > 0) {
    close(epoll_fd);
  }
  return;
}

static void* thread_io_worker(void* arg) {
  struct epoll_event events[MAX_EVENTS];
  int wait_time = -1, nfds = 0;
  while (1) {
    nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, wait_time);
    if (nfds == -1) {
      ACTOR_MSLEEP(100);
      continue;
    }
    for (int i = 0; i < nfds; i++) {
      ACTOR_PRINT("ready fd[%d] event[%X]\n", events[i].data.fd,
                  events[i].events);
      process_io((actor_io_t*)events[i].data.ptr, events[i].events);
    }
    ACTOR_MSLEEP(5);
  }
  return NULL;
}

static void process_io(actor_io_t* io, int event) {
  switch (io->type) {
    case TCP_SERVICE:
      break;
    case TCP_CLIENT:
      break;
    case UDP:
      break;
    case SERIAL:
      break;
    default:
      break;
  }
  return 0;
}