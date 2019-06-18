#ifndef __ACTOR_SERIAL_H__
#define __ACTOR_SERIAL_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
#include "actor_common.h"

typedef struct actor_serial {
  char name[32];
  int baudrate;
  int timeout;
  actor_io_t* io;
  char bits;
  char parity;
  char stop;
} actor_serial_t;

extern actor_serial_t* open_serial(char* name,
                                   struct actor_context* context,
                                   int send_buf_len,
                                   int recv_buf_len,
                                   int timeout);

extern int config_serial(actor_serial_t* serial,
                         int baudrate,
                         char bits,
                         char parity,
                         char stop);

extern int close_serial(actor_serial_t* serial);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif