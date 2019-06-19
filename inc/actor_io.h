#ifndef __ACTOR_IO_H__
#define __ACTOR_IO_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
#include "actor_def.h"
#include "actor_list.h"
#include "actor_spinlock.h"

enum actor_io_type {
  ACTOR_IO_TCP_SERVICE = 0,
  ACTOR_IO_TCP_CLIENT,
  ACTOR_IO_UDP,
  ACTOR_IO_SERIAL,
  ACTOR_IO_PIPE
};

typedef struct actor_io {
  unsigned short recv_r;
  unsigned short recv_w;
  unsigned short send_r;
  unsigned short send_w;
  unsigned short recv_buf_len;
  unsigned short send_buf_len;
  unsigned short timeout;
  char* recv_buf;
  char* send_buf;
  struct actor_context* context;
  int fd;
  enum actor_io_type type;
  actor_tick_t time;
  alist_node_t list;
  struct actor_spinlock lock;
  int event;
} actor_io_t;

extern void actor_io_init(void);
extern void actor_io_deinit(void);

extern int actor_io_fd_add(actor_io_t* io);

extern int actor_io_fd_delete(actor_io_t* io);
extern int actor_io_fd_write(actor_io_t* io, int enable);
extern int actor_io_write(actor_io_t* io, void* buf, int len);
extern actor_io_t* create_io(int send_buf_len, int recv_buf_len);
extern int delete_io(actor_io_t* io);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif