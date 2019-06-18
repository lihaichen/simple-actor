
#include "actor_io.h"
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <unistd.h>
#include "actor_common.h"
#include "actor_def.h"
#include "actor_spinlock.h"

#define MAX_EVENTS 2

static void* thread_io_worker(void* arg);
static int process_io(actor_io_t* io, int event);

static int epoll_fd;
pthread_t pid;

struct actor_io_node {
  alist_node_t list;
  int fd_pipe;
  struct actor_spinlock lock;
};

static struct actor_io_node G_NODE;

int actor_io_add(actor_io_t* io) {
  struct epoll_event ev;
  ACTOR_ASSERT(io != NULL);
  ACTOR_SPIN_LOCK(&G_NODE);
  alist_insert_after(&G_NODE.list, &io->list);
  ACTOR_SPIN_UNLOCK(&G_NODE);
  ev.events = EPOLLIN;
  ev.data.ptr = io;
  int flags = fcntl(io->fd, F_GETFL, 0);
  fcntl(io->fd, F_SETFL, flags);
  return epoll_ctl(epoll_fd, EPOLL_CTL_ADD, io->fd, &ev);
}

int actor_io_del(actor_io_t* io) {
  ACTOR_SPIN_LOCK(&G_NODE);
  alist_remove(&io->list);
  ACTOR_SPIN_UNLOCK(&G_NODE);
  return epoll_ctl(epoll_fd, EPOLL_CTL_DEL, io->fd, NULL);
}

int actor_io_write(actor_io_t* io, int enable) {
  struct epoll_event ev;
  ev.events = EPOLLIN | (enable ? EPOLLOUT : 0);
  ev.data.ptr = io;
  return epoll_ctl(epoll_fd, EPOLL_CTL_MOD, io->fd, &ev);
}

void actor_io_init(void) {
  ACTOR_SPIN_INIT(&G_NODE);
  alist_init(&G_NODE.list);
  epoll_fd = epoll_create(1);
  if (epoll_fd < 0) {
    ACTOR_PRINT("epoll create %s\n", strerror(errno));
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
  ACTOR_SPIN_DESTROY(&G_NODE);
  return;
}

static void* thread_io_worker(void* arg) {
  struct epoll_event events[MAX_EVENTS];
  int wait_time = 100, nfds = 0;
  while (1) {
    nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, wait_time);
    if (nfds == -1) {
      ACTOR_MSLEEP(100);
      continue;
    }
    if (nfds == 0) {
      // timeout
      ACTOR_SPIN_LOCK(&G_NODE);
      alist_node_t* list = &G_NODE.list;
      actor_io_t* io = NULL;
      actor_tick_t tick;
      ACTOR_GET_TICK(&tick);
      for (struct alist_node* node = list->next; node != list;
           node = node->next) {
        io = acontainer_of(node, struct actor_io, list);
        if (io->type == SERIAL && io->recv_r != io->recv_w) {
          if (tick >= io->time + io->timeout && io->time != 0) {
            io->time = 0;
            ACTOR_PRINT("timeout len[%d]\n", io->recv_w - io->recv_r);
          }
        }
      }
      ACTOR_SPIN_UNLOCK(&G_NODE);
      continue;
    }
    for (int i = 0; i < nfds; i++) {
      actor_io_t* io = (actor_io_t*)events[i].data.ptr;
      ACTOR_PRINT("ready fd[%d] event[%X]\n", io->fd, events[i].events);
      wait_time = process_io(io, events[i].events);
    }
    ACTOR_MSLEEP(5);
  }
  return NULL;
}

static int process_io(actor_io_t* io, int event) {
  int timeout = -1;
  char buf[128];

  int tmp = 0;
  switch (io->type) {
    case TCP_SERVICE:
      break;
    case TCP_CLIENT:
      break;
    case UDP:
      break;
    case SERIAL: {
      do {
        memset(buf, 0, sizeof(buf));
        tmp = read(io->fd, buf, sizeof(buf));
        if (tmp < 1) {
          timeout = io->timeout;
          break;
        }
        for (int i = 0; i < tmp; i++) {
          io->recv_buf[io->recv_w++] = buf[i];
          if (io->recv_w == io->recv_r) {
            io->recv_r++;
            io->recv_r %= io->recv_buf_len;
          }
          io->recv_w %= io->send_buf_len;
        }
        ACTOR_GET_TICK(&io->time);
      } while (tmp > 0);
    } break;
    default:
      break;
  }
  return timeout;
}