#include <criterion/criterion.h>
#include <stdio.h>
#include <string.h>
#include "actor_server.h"

Test(actor_server, add) {
#define ADD_SIZE 30
  actor_server_init();
  struct actor_context* test1 = actor_context_new("test1", NULL, NULL);
  cr_assert(test1 != NULL, "actor_context_new null");
  cr_assert(actor_context_total() == 1, "context_total error\n");
  actor_context_release(test1);
  cr_assert(actor_context_total() == 0, "context_total error\n");
  struct actor_context* ctx[ADD_SIZE];
  char buf[16];
  for (int i = 0; i < ADD_SIZE; i++) {
    memset(buf, 0, sizeof(buf));
    snprintf(buf, 15, "test%d", i);
    ctx[i] = actor_context_new(buf, NULL, NULL);
    cr_assert(ctx[i] != NULL, "actor_context_new null");
  }
  cr_assert(actor_context_total() == ADD_SIZE, "context_total error\n");
  struct actor_context* res;
  for (int i = 0; i < ADD_SIZE; i++) {
    memset(buf, 0, sizeof(buf));
    snprintf(buf, 15, "test%d", i);
    res = actor_context_find(buf);
    cr_assert(res != NULL, "actor_context_find null");
    cr_assert(res == ctx[i], "actor_context_find null");
  }

  res = actor_context_find("test");
  cr_assert(res == NULL, "actor_context_find test");
  for (int i = 0; i < ADD_SIZE; i++) {
    actor_context_release(ctx[i]);
  }
  cr_assert(actor_context_total() == 0, "context_total error\n");
  for (int i = 0; i < ADD_SIZE; i++) {
    memset(buf, 0, sizeof(buf));
    snprintf(buf, 15, "test%d", i);
    res = actor_context_find(buf);
    cr_assert(res == NULL, "actor_context_find null");
  }
  actor_server_deinit();
}
