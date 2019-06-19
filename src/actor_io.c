
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
static int process_io_timeout();

static int epoll_fd;
pthread_t pid;

struct actor_io_node {
  alist_node_t list;
  actor_pipe_t* pipe;
  struct actor_spinlock lock;
};

static struct actor_io_node G_NODE;

int actor_io_add(actor_io_t* io) {
  struct epoll_event ev;
  int res = 0;
  ACTOR_ASSERT(io != NULL);
  ACTOR_SPIN_LOCK(&G_NODE);
  alist_insert_after(&G_NODE.list, &io->list);
  ACTOR_SPIN_UNLOCK(&G_NODE);
  ev.events = EPOLLIN;
  ev.data.ptr = io;
  int flags = fcntl(io->fd, F_GETFL, 0);
  flags |= O_NONBLOCK;
  fcntl(io->fd, F_SETFL, flags);
  res = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, io->fd, &ev);
  if (res < 0) {
    ACTOR_PRINT("epoll_ctl add[%d] %s\n", io->fd, strerror(errno));
  }
  if (G_NODE.pipe != NULL) {
    if (write(G_NODE.pipe->fd[1], "A", 1) != 1) {
      ACTOR_PRINT("actor_io_add write pipe %d %s\n", errno, strerror(errno));
    }
  }
  return res;
}

int actor_io_del(actor_io_t* io) {
  ACTOR_SPIN_LOCK(&G_NODE);
  alist_remove(&io->list);
  ACTOR_SPIN_UNLOCK(&G_NODE);
  int res = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, io->fd, NULL);
  if (res < 0) {
    ACTOR_PRINT("epoll_ctl del[%d] %s\n", io->fd, strerror(errno));
  }
  // if (G_NODE.pipe != NULL) {
  //   if (write(G_NODE.pipe->fd[1], "R", 1) != 1) {
  //     ACTOR_PRINT("actor_io_del write pipe %d %s\n", errno, strerror(errno));
  //   }
  // }
  return res;
}

int actor_io_write(actor_io_t* io, int enable) {
  struct epoll_event ev;
  ev.events = EPOLLIN | (enable ? EPOLLOUT : 0);
  ev.data.ptr = io;
  int res = epoll_ctl(epoll_fd, EPOLL_CTL_MOD, io->fd, &ev);
  if (res < 0) {
    ACTOR_PRINT("epoll_ctl write[%d][%d] %s\n", io->fd, enable,
                strerror(errno));
  }
  if (G_NODE.pipe != NULL) {
    if (write(G_NODE.pipe->fd[1], "M", 1) != 1) {
      ACTOR_PRINT("actor_io_write write pipe %d %s\n", errno, strerror(errno));
    }
  }
  return res;
}

void actor_io_init(void) {
  ACTOR_SPIN_INIT(&G_NODE);
  alist_init(&G_NODE.list);
  epoll_fd = epoll_create(1);
  if (epoll_fd < 0) {
    ACTOR_PRINT("epoll create %s\n", strerror(errno));
    return;
  }
  G_NODE.pipe = create_pipe(NULL, 16, 16, 20);
  if (G_NODE.pipe == NULL) {
    ACTOR_PRINT("io create pipe error\n");
  }

  // create io thread
  pthread_create(&pid, NULL, thread_io_worker, NULL);
  return;
}

void actor_io_deinit(void) {
  pthread_cancel(pid);
  destroy_pipe(G_NODE.pipe);
  if (epoll_fd > 0) {
    close(epoll_fd);
  }
  ACTOR_SPIN_DESTROY(&G_NODE);
  return;
}

static void* thread_io_worker(void* arg) {
  struct epoll_event events[MAX_EVENTS];
  int wait_time = -1, nfds = 0;
  while (1) {
    ACTOR_PRINT("epoll_wait time %d\n", wait_time);
    nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, wait_time);
    if (nfds == -1) {
      ACTOR_PRINT("epoll_wait error %d %s\n", errno, strerror(errno));
      ACTOR_MSLEEP(100);
      continue;
    }
    if (nfds == 0) {
      // timeout
      wait_time = process_io_timeout();
      continue;
    }
    for (int i = 0; i < nfds; i++) {
      actor_io_t* io = (actor_io_t*)events[i].data.ptr;
      // ACTOR_PRINT("ready fd[%d] event[%X]\n", io->fd, events[i].events);
      wait_time = process_io(io, events[i].events);
    }
    ACTOR_MSLEEP(5);
  }
  return NULL;
}

static int process_io_timeout() {
  alist_node_t* list = &G_NODE.list;
  int wait_time = -1;
  actor_io_t* io = NULL;
  actor_tick_t tick = ACTOR_GET_TICK(NULL);
  ACTOR_SPIN_LOCK(&G_NODE);
  for (struct alist_node* node = list->next; node != list; node = node->next) {
    io = acontainer_of(node, struct actor_io, list);
    if ((io->type == ACTOR_IO_SERIAL || io->type == ACTOR_IO_PIPE) &&
        io->recv_r != io->recv_w) {
      if (IS_TIMEOUT(tick, io->time + io->timeout)) {
        ACTOR_PRINT("timeout len[%d]\n", io->recv_w - io->recv_r);
        io->recv_r = io->recv_w = 0;
      } else {
        int diff = tick - io->time - io->timeout;
        wait_time = (wait_time < 0) || (wait_time < diff) ? diff : wait_time;
      }
    }
  }
  ACTOR_SPIN_UNLOCK(&G_NODE);
  return wait_time;
}

static int process_io(actor_io_t* io, int event) {
  int timeout = -1;
  char buf[128];
  int tmp = 0;
  do {
    memset(buf, 0, sizeof(buf));
    tmp = read(io->fd, buf, sizeof(buf));
    if (tmp < 1) {
      switch (io->type) {
        case ACTOR_IO_TCP_SERVICE:
          break;
        case ACTOR_IO_TCP_CLIENT:
          break;
        case ACTOR_IO_UDP:
          break;
        case ACTOR_IO_PIPE:
        case ACTOR_IO_SERIAL: {
          timeout = io->timeout;
        } break;
        default:
          break;
      }
      ACTOR_PRINT("read[%d] error %d\n", io->fd, errno);
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
  return timeout;
}

actor_io_t* create_io(int send_buf_len, int recv_buf_len) {
  actor_io_t* io = ACTOR_MALLOC(sizeof(actor_io_t));
  ACTOR_ASSERT(io != NULL);
  memset(io, 0, sizeof(actor_io_t));
  io->recv_buf_len = recv_buf_len;
  io->send_buf_len = send_buf_len;
  io->recv_buf = ACTOR_MALLOC(io->recv_buf_len);
  ACTOR_ASSERT(io->recv_buf != NULL);
  io->send_buf = ACTOR_MALLOC(io->send_buf_len);
  ACTOR_ASSERT(io->send_buf != NULL);
  alist_init(&io->list);
  return io;
}

int delete_io(actor_io_t* io) {
  ACTOR_ASSERT(io != NULL);
  ACTOR_FREE(io->send_buf);
  ACTOR_FREE(io->recv_buf);
  return 0;
}