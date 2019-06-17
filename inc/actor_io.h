#ifndef __ACTOR_IO_H__
#define __ACTOR_IO_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

enum actor_io_type { TCP_SERVICE = 0, TCP_CLIENT, UDP, SERIAL };

typedef struct actor_io {
  unsigned short recv_r;
  unsigned short recv_w;
  unsigned short send_r;
  unsigned short send_w;
  unsigned short recv_buf_len;
  unsigned short send_buf_len;
  char* recv_buf;
  char* send_buf;
  int fd;
  int event;
  enum actor_io_type type;
} actor_io_t;

extern void actor_io_init(void);
extern void actor_io_deinit(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif