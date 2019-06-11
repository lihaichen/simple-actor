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
  printf("type[%X] session[%X] source[%p] msg[%s]\n", type, session, source,
         (char*)msg);
  return 0;
}
int main() {
  actor_start(1);
  struct actor_context* ctx1 = actor_context_new("actor1", NULL);
  struct actor_context* ctx2 = actor_context_new("actor2", NULL);
  actor_context_callback(ctx1, print_cb, NULL);
  actor_context_callback(ctx2, print_cb, NULL);
  printf("ctx1 %p ctx2 %p, sizeof(int)%ld\n", ctx1, ctx2, sizeof(int));
  
  actor_context_send(ctx2, ctx2, 0, 0, "hello1", 5);
  actor_context_send(ctx1, ctx1, 0, 0, "hello2", 5);
  actor_context_send(ctx1, ctx2, 0, 0, "hello3", 5);
  sleep(1);
  for(int i=0; i < 10; i++) {
    actor_context_send(ctx1, ctx1, 0, i, "hello4", 5);
    actor_context_send(ctx1, ctx2, 0, i, "hello4", 5);
  }
  
  sleep(10);
  printf("end\n");
}
// Test(main, process) {
//   actor_start(1);
//   struct actor_context* ctx1 = actor_context_new("actor1", NULL);
//   struct actor_context* ctx2 = actor_context_new("actor1", NULL);
//   cr_assert(1, "test 0");
//   cr_assert(ctx1 != NULL, "actor_context_new error");
//   cr_assert(ctx2 != NULL, "actor_context_new error");

//   actor_callback(ctx1, print_cb, NULL);
//   actor_callback(ctx2, print_cb, NULL);
//   printf("ctx1 %p ctx2 %p, sizeof(int)%d\n", ctx1, ctx2, sizeof(int));
//   actor_send(ctx1, ctx2, ctx1, 0, 0, "hello", 5);
//   sleep(10);
//   printf("end\n");
//   // actor_context_release(ctx1);
//   // actor_context_release(ctx2);
// }
