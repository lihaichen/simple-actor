// #include <criterion/criterion.h>
#include <stdio.h>
#include "actor_common.h"
#include "actor_server.h"
# include <unistd.h>

struct actor_context;
int print_cb(struct actor_context* context,
             void* ud,
             int type,
             int session,
             int source,
             const void* msg,
             int sz) {
  printf("ctx %p\n", context);
  printf("type[%X] session[%X] source[%p] msg[%s]\n", type, session, source, msg);
  return 0;
}
int main(){
   actor_start(1);
  struct actor_context* ctx1 = actor_context_new("actor1", NULL);
  struct actor_context* ctx2 = actor_context_new("actor1", NULL);
  actor_callback(ctx1, print_cb, NULL);
  actor_callback(ctx2, print_cb, NULL);
  printf("ctx1 %p ctx2 %p, sizeof(int)%d\n", ctx1, ctx2, sizeof(int));
  actor_send(ctx1, ctx2, ctx1, 0, 0, "hello", 5);
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
