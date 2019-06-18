#ifndef __ACTOR_IO_H__
#define __ACTOR_IO_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
#include "actor_def.h"
#include "actor_list.h"

enum actor_io_type { TCP_SERVICE = 0, TCP_CLIENT, UDP, SERIAL };

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
  int event;
  enum actor_io_type type;
  actor_tick_t time;
  alist_node_t list;
} actor_io_t;

extern void actor_io_init(void);
extern void actor_io_deinit(void);

extern int actor_io_add(actor_io_t* io);

extern int actor_io_del(actor_io_t* io);
extern int actor_io_write(actor_io_t* io, int enable);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif