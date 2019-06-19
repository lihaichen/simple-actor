
#include "actor_io.h"
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <unistd.h>
#include "actor_common.h"
#include "actor_def.h"
#include "actor_spinlock.h"

#define MAX_EVENTS 2

static void* thread_io_worker(void* arg);
static int process_io_timeout();
static int process_io_recv(actor_io_t* io);
static int process_io_send(actor_io_t* io);
static struct pollfd* generate_pollfd(struct pollfd* last_fds);

struct actor_io_node {
  alist_node_t list;
  actor_pipe_t* pipe;
  struct actor_spinlock lock;
  int poll_sum;
  pthread_t pid;
};

static struct actor_io_node G_NODE;

int actor_io_fd_add(actor_io_t* io) {
  int res = 0;
  ACTOR_ASSERT(io != NULL);
  ACTOR_SPIN_LOCK(&G_NODE);
  alist_insert_after(&G_NODE.list, &io->list);
  G_NODE.poll_sum++;
  ACTOR_SPIN_UNLOCK(&G_NODE);
  io->event = POLLIN;
  int flags = fcntl(io->fd, F_GETFL, 0);
  flags |= O_NONBLOCK;
  fcntl(io->fd, F_SETFL, flags);
  if (G_NODE.pipe != NULL) {
    if (write(G_NODE.pipe->fd[1], "A", 1) != 1) {
      ACTOR_PRINT("actor_io_fd_add write pipe %d %s\n", errno, strerror(errno));
    }
  }
  ACTOR_PRINT("actor_io_fd_add %d sum %d\n", io->fd, G_NODE.poll_sum);
  return res;
}

int actor_io_fd_delete(actor_io_t* io) {
  int res = 0;
  ACTOR_SPIN_LOCK(&G_NODE);
  if (!alist_isempty(&io->list)) {
    alist_remove(&io->list);
    G_NODE.poll_sum--;
  }
  ACTOR_SPIN_UNLOCK(&G_NODE);
  if (G_NODE.pipe != NULL) {
    if (write(G_NODE.pipe->fd[1], "R", 1) != 1) {
      ACTOR_PRINT("actor_io_fd_delete write pipe %d %s\n", errno,
                  strerror(errno));
    }
  }
  ACTOR_PRINT("actor_io_fd_delete %d\n", io->fd);
  return res;
}
// this must use io lock in caller
int actor_io_fd_write(actor_io_t* io, int enable) {
  int res = 0;
  io->event = POLLIN | (enable ? POLLOUT : 0);
  if (G_NODE.pipe != NULL) {
    if (write(G_NODE.pipe->fd[1], "M", 1) != 1) {
      ACTOR_PRINT("actor_io_write write pipe %d %s\n", errno, strerror(errno));
    }
  }
  ACTOR_PRINT("actor_io_fd_write %d enable[%d]\n", io->fd, enable);
  return res;
}

void actor_io_init(void) {
  ACTOR_SPIN_INIT(&G_NODE);
  alist_init(&G_NODE.list);
  G_NODE.pipe = create_pipe(NULL, 16, 16, 20);
  if (G_NODE.pipe == NULL) {
    ACTOR_PRINT("io create pipe error\n");
  }
  // create io thread
  pthread_create(&G_NODE.pid, NULL, thread_io_worker, NULL);
  return;
}

void actor_io_deinit(void) {
  pthread_cancel(G_NODE.pid);
  ACTOR_SPIN_UNLOCK(&G_NODE);
  destroy_pipe(G_NODE.pipe);
  ACTOR_SPIN_DESTROY(&G_NODE);
  return;
}

static struct pollfd* generate_pollfd(struct pollfd* last_fds) {
  struct pollfd* fds = NULL;
  actor_io_t* io = NULL;
  ACTOR_SPIN_LOCK(&G_NODE);
  alist_node_t* list = &G_NODE.list;
  if (last_fds != NULL)
    ACTOR_FREE(last_fds);
  fds = ACTOR_MALLOC(sizeof(struct pollfd) * G_NODE.poll_sum);
  if (fds == NULL) {
    goto breakout;
  }
  G_NODE.poll_sum = 0;
  for (struct alist_node* node = list->next; node != list; node = node->next) {
    io = acontainer_of(node, struct actor_io, list);
    fds[G_NODE.poll_sum].fd = io->fd;
    fds[G_NODE.poll_sum].events = io->event;
    fds[G_NODE.poll_sum].revents = 0;
    G_NODE.poll_sum++;
  }
breakout:
  ACTOR_SPIN_UNLOCK(&G_NODE);
  return fds;
}

static void* thread_io_worker(void* arg) {
  struct pollfd* fds = NULL;
  alist_node_t* list = &G_NODE.list;
  actor_io_t* io = NULL;
  int wait_time = -1, nfds = 0;
  while (1) {
    if (G_NODE.poll_sum < 1) {
      ACTOR_MSLEEP(100);
      continue;
    }
    fds = generate_pollfd(fds);
    if (fds == NULL) {
      ACTOR_MSLEEP(100);
      continue;
    }
    ACTOR_PRINT("poll time %d sum %d\n", wait_time, G_NODE.poll_sum);
    nfds = poll(fds, G_NODE.poll_sum, wait_time);
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
      ACTOR_PRINT("ready fd[%d] event[%d]\n", fds[i].fd, fds[i].revents);
      io = NULL;
      ACTOR_SPIN_LOCK(&G_NODE);
      for (struct alist_node* node = list->next; node != list;
           node = node->next) {
        actor_io_t* tmp = acontainer_of(node, struct actor_io, list);
        if (tmp->fd == fds[i].fd) {
          io = tmp;
          break;
        }
      }
      ACTOR_SPIN_UNLOCK(&G_NODE);
      if (io == NULL) {
        ACTOR_PRINT("poll fd not find %d\n", fds[i].fd);
        continue;
      }

      if (fds[i].revents & POLLIN) {
        int t = process_io_recv(io);
        wait_time = (wait_time < 0) || (wait_time < t) ? t : wait_time;
      }
      if (fds[i].revents & POLLOUT) {
        process_io_send(io);
      }
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
        int len = io->recv_w - io->recv_r;
        if (len < 0)
          len += io->recv_buf_len;
        ACTOR_PRINT("timeout fd[%d] len[%d] %d %d\n", io->fd, len, io->recv_r,
                    io->recv_w);
        ACTOR_PRINT("==>%s\n", io->recv_buf);
        if (io->context) {
          char* tmp = ACTOR_MALLOC(len + 1);
          if (tmp != NULL) {
            memset(tmp, 0, len + 1);
            for (int i = 0; i < len; i++) {
              tmp[i] = io->recv_buf[io->recv_r++];
              io->recv_r %= io->recv_buf_len;
            }
            actor_context_send(NULL, io->context, ACTOR_MSG_TYPE_TEXT, 1234,
                               tmp, len);
            ACTOR_FREE(tmp);
          }

        } else {
          ACTOR_PRINT("io malloc[%d] null\n", len);
        }
        io->recv_r = io->recv_w = 0;
        memset(io->recv_buf, 0, io->recv_buf_len);
      } else {
        int diff = tick - io->time - io->timeout;
        wait_time = (wait_time < 0) || (wait_time < diff) ? diff : wait_time;
      }
    }
  }
  ACTOR_SPIN_UNLOCK(&G_NODE);
  return wait_time;
}

static int process_io_recv(actor_io_t* io) {
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
    ACTOR_SPIN_LOCK(io);
    for (int i = 0; i < tmp; i++) {
      io->recv_buf[io->recv_w++] = buf[i];
      if (io->recv_w == io->recv_r) {
        io->recv_r++;
        io->recv_r %= io->recv_buf_len;
      }
      io->recv_w %= io->recv_buf_len;
    }
    ACTOR_SPIN_UNLOCK(io);
    ACTOR_GET_TICK(&io->time);
  } while (tmp > 0);
  return timeout;
}

static int process_io_send(actor_io_t* io) {
  ACTOR_SPIN_LOCK(io);
  if (io->send_r > io->send_w) {
    int len = io->send_buf_len - io->send_r + 1;
    if (write(io->fd, io->send_buf + io->send_r, len) != len) {
      goto breakout;
    }
    io->send_r = 0;
  }
  if (io->send_r < io->send_w) {
    int len = io->send_w - io->send_r;
    if (write(io->fd, io->send_buf + io->send_r, len) != len) {
      goto breakout;
    }
    io->send_r = io->send_w = 0;
    actor_io_fd_write(io, 0);
  }
breakout:
  ACTOR_SPIN_UNLOCK(io);
  return -1;
}

int actor_io_write(actor_io_t* io, void* buf, int len) {
  int i = 0, remain = 0;
  ACTOR_SPIN_LOCK(io);
  remain = io->send_r - io->send_w;
  if (remain <= 0)
    remain += io->send_buf_len;
  remain--;
  remain = remain > len ? len : remain;
  for (i = 0; i < remain; i++) {
    io->send_buf[io->send_w++] = ((char*)buf)[i];
    io->send_w %= io->send_buf_len;
  }
  actor_io_fd_write(io, 1);
  ACTOR_SPIN_UNLOCK(io);
  return remain;
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
  ACTOR_SPIN_INIT(io);
  return io;
}

int delete_io(actor_io_t* io) {
  ACTOR_ASSERT(io != NULL);
  ACTOR_FREE(io->send_buf);
  ACTOR_FREE(io->recv_buf);
  ACTOR_SPIN_DESTROY(io);
  return 0;
}