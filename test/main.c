// #include <criterion/criterion.h>
#include <stdio.h>
#include <unistd.h>
#include "actor_common.h"
#include "actor_server.h"
#include "actor_timer.h"

struct actor_context;

int print_cb(struct actor_context* context,
             void* ud,
             int type,
             int session,
             void* source,
             const void* msg,
             int sz) {
  if (type == ACTOR_MSG_TYPE_INIT) {
    printf("[%s] init...\n", actor_context_name(context));
    return 0;
  }
  unsigned long long ms = 0;
  ACTOR_GET_TICK(&ms);
  printf("ctx %p name[%s] time[%lld] ===>\n", context,
         actor_context_name(context), ms % 100000);
  printf("type[%X] session[%d] source[%p] msg[%s]\n", type, session, source,
         (char*)msg);
  usleep(100 * 1000);
  return 0;
}

static int rfid_cb(struct actor_context* context,
                   void* ud,
                   int type,
                   int session,
                   void* source,
                   const void* msg,
                   int sz) {
  actor_serial_t* serial = NULL;
  if (type == ACTOR_MSG_TYPE_INIT) {
    printf("[%s] init...\n", actor_context_name(context));
    serial = open_serial("/dev/ttyUSB0", context, 512, 512, 20);
    if (serial == NULL) {
      ACTOR_PRINT("open_serial null\n");
      return 0;
    }
    if (config_serial(serial, 115200, 8, 'n', 1) != 0) {
      ACTOR_PRINT("config_serial error\n");
      return 0;
    }
    char tmp[] = {0xA0, 0x03, 0x01, 0x70, 0xEC};
    actor_io_write(serial->io, (void*)tmp, sizeof(tmp));
    return 0;
  }
  if (type == ACTOR_MSG_TYPE_IO) {
    printf("recv==>");
    for (int i = 0; i < sz; i++) {
      unsigned char* p = msg;
      printf("%02X ", p[i]);
    }
    printf("\n");
  }
  return 0;
}

int main() {
  actor_start(3);
  struct actor_context* ctx1 = actor_context_new("actor1", print_cb, NULL);
  struct actor_context* ctx2 = actor_context_new("actor2", print_cb, NULL);
  struct actor_context* ctx3 = actor_context_new("actor3", print_cb, NULL);
  actor_context_new("rfid", rfid_cb, NULL);

  //   actor_context_callback(ctx1, print_cb, NULL);
  //   actor_context_callback(ctx2, print_cb, NULL);
  //   actor_context_callback(ctx3, print_cb, NULL);

  // actor_timer_add(ctx1, 1, 1000, ACTOR_TIMER_FLAG_PERIOD);
  // actor_timer_add(ctx2, 2, 2000, ACTOR_TIMER_FLAG_PERIOD);
  // actor_timer_add(ctx3, 3, 3000, ACTOR_TIMER_FLAG_PERIOD);

  // actor_timer_add(ctx1, 4, 1000, 0);
  // actor_timer_add(ctx2, 5, 1000, 0);
  // actor_timer_add(ctx3, 6, 1000, 0);

  // actor_timer_add(ctx1, 7, 0, 0);
  // actor_timer_add(ctx2, 8, 0, 0);
  // actor_timer_add(ctx3, 9, 0, 0);

  // actor_context_send(ctx2, ctx2, 0, 0, "hello1", 5);
  // actor_context_send(ctx1, ctx1, 0, 0, "hello2", 5);
  // actor_context_send(ctx1, ctx2, 0, 0, "hello3", 5);
  // actor_context_send(ctx1, ctx3, 0, 0, "hello3", 5);
  // actor_context_send(ctx1, ctx3, 0, 0, "hello3", 5);
  // sleep(1);

  // char buf[16];
  // for (int i = 10; i < 20; i++) {
  //   memset(buf, 0, sizeof(buf));
  //   sprintf(buf, "hello actor1 %02d", i);
  //   actor_context_send(ctx1, ctx1, 0, i, buf, strlen(buf));
  //   memset(buf, 0, sizeof(buf));
  //   sprintf(buf, "hello actor2 %02d", i);
  //   actor_context_send(ctx1, ctx2, 0, i, buf, strlen(buf));
  //   memset(buf, 0, sizeof(buf));
  //   sprintf(buf, "hello actor3 %02d", i);
  //   actor_context_send(ctx1, ctx3, 0, i, buf, strlen(buf));
  // }

  // actor_serial_t* serial = open_serial("/dev/ttyUSB0", ctx1, 512, 512, 20);
  // if (serial == NULL) {
  //   ACTOR_PRINT("open_serial null\n");
  //   goto breakout;
  // }
  // if (config_serial(serial, 115200, 8, 'n', 1) != 0) {
  //   ACTOR_PRINT("config_serial error\n");
  //   goto breakout;
  // }
  // const char* s = "This is serial test code";
#if 0
  actor_io_write(serial->io, (void*)s, strlen(s));
#elif 0
  int len = actor_io_write_direct(serial->io, (void*)s, strlen(s));
  printf("send[%d] len %d\n", serial->io->fd, len);
  ACTOR_MSLEEP(30);
  len = actor_io_write_direct(serial->io, (void*)s, strlen(s));
  printf("send[%d] len %d\n", serial->io->fd, len);
#else
#endif

  sleep(5);
  // close_serial(serial);

breakout:
  printf("end\n");
  actor_stop();
}