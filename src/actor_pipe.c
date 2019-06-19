#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include "actor_common.h"

actor_pipe_t* create_pipe(struct actor_context* context,
                          int send_buf_len,
                          int recv_buf_len,
                          int timeout) {
  actor_io_t* io = NULL;
  actor_pipe_t* ap = ACTOR_MALLOC(sizeof(*ap));
  ACTOR_ASSERT(ap != NULL);
  memset(ap, 0, sizeof(*ap));
  io = create_io(send_buf_len, recv_buf_len);
  if (io == NULL) {
    ACTOR_FREE(ap);
    return NULL;
  }
  io->timeout = timeout;
  io->context = context;
  io->type = ACTOR_IO_PIPE;
  ap->io = io;
  if (pipe(ap->fd) < 0) {
    ACTOR_PRINT("create pipe %s\n", strerror(errno));
    delete_io(io);
    ACTOR_FREE(ap);
    return NULL;
  }
  io->fd = ap->fd[0];
  actor_io_add(io);
  return ap;
}

int destroy_pipe(actor_pipe_t* ap) {
  actor_io_del(ap->io);
  close(ap->fd[0]);
  close(ap->fd[1]);
  delete_io(ap->io);
  ACTOR_FREE(ap);
  return 0;
}