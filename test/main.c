// #include <criterion/criterion.h>
#include <stdio.h>
#include <unistd.h>
#include "actor_common.h"
#include "actor_server.h"

struct actor_context;
int print_cb(struct actor_context* context,
             void* ud,
             int type,
             int session,
             void* source,
             const void* msg,
             int sz) {
  printf("ctx %p name[%s]\n", context, actor_context_name(context));
  printf("type[%X] session[%d] source[%p] msg[%s]\n", type, session, source,
         (char*)msg);
  usleep(100 * 1000);
  return 0;
}
int main() {
  actor_start(3);
  struct actor_context* ctx1 = actor_context_new("actor1", NULL);
  struct actor_context* ctx2 = actor_context_new("actor2", NULL);
  struct actor_context* ctx3 = actor_context_new("actor3", NULL);
  actor_context_callback(ctx1, print_cb, NULL);
  actor_context_callback(ctx2, print_cb, NULL);
  actor_context_callback(ctx3, print_cb, NULL);

  actor_context_send(ctx2, ctx2, 0, 0, "hello1", 5);
  actor_context_send(ctx1, ctx1, 0, 0, "hello2", 5);
  actor_context_send(ctx1, ctx2, 0, 0, "hello3", 5);
  actor_context_send(ctx1, ctx3, 0, 0, "hello3", 5);
  actor_context_send(ctx1, ctx3, 0, 0, "hello3", 5);
  sleep(1);
  for (int i = 0; i < 10; i++) {
    actor_context_send(ctx1, ctx1, 0, i, "hello11", 7);
    actor_context_send(ctx1, ctx2, 0, i, "hello22", 7);
    actor_context_send(ctx1, ctx3, 0, i, "hello33", 7);
  }

  sleep(10);
  printf("end\n");
}