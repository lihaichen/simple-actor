#ifndef __ACTOR_PIPE_H__
#define __ACTOR_PIPE_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
#include "actor_common.h"

typedef struct actor_pipe {
  actor_io_t* io;
  int fd[2];
} actor_pipe_t;

extern actor_pipe_t* create_pipe(struct actor_context* context,
                                 int send_buf_len,
                                 int recv_buf_len,
                                 int timeout);

extern int destroy_pipe(actor_pipe_t* ap);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif