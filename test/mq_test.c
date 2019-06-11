#include <criterion/criterion.h>
#include <stdio.h>
#include <string.h>
#include "actor_mq.h"

#define ADD_SIZE 30

Test(actor_mq, global_mq) {
  int i = 0;
  struct actor_message_queue* mq[ADD_SIZE];
  actor_globalmq_init();

  for (i = 0; i < ADD_SIZE; i++) {
    mq[i] = actor_mq_create(NULL);
    cr_assert_not_null(mq[i], "actor_mq_create null");
    actor_globalmq_push(mq[i]);
  }
  i = 0;
  while (1) {
    struct actor_message_queue* tmp = actor_globalmq_pop();
    if (tmp == NULL) {
      break;
    }
    cr_expect_eq(mq[i], tmp, "actor_globalmq_pop not eq %p %p", mq[i], tmp);
    actor_mq_release(tmp);
    i++;
  }
  cr_expect_eq(i, ADD_SIZE, "actor_globalmq_pop sum not eq");

  actor_globalmq_deinit();
  return;
}

Test(actor_mq, second_mq) {
  actor_globalmq_init();
  struct actor_message_queue* mq = actor_mq_create(NULL);
  int test_count[] = {5, 15, 40, 60, 90};
  cr_assert_not_null(mq, "actor_mq_create null");
  struct actor_message msg, tmp;
  memset(&msg, 0, sizeof(msg));
  memset(&tmp, 0, sizeof(tmp));
  msg.session = 0xAA;

  actor_mq_push(mq, &msg);
  cr_expect_eq(actor_mq_length(mq), 1, "second len not eq");
  actor_mq_pop(mq, &tmp);
  cr_expect_eq(msg.session, tmp.session, "second push pop not eq");
  cr_expect_eq(actor_mq_length(mq), 0, "second len not eq");

  for (int j = 0; j < sizeof(test_count) / sizeof(int); j++) {
    for (int i = 0; i < test_count[j]; i++) {
      msg.session = 0xAA + i;
      actor_mq_push(mq, &msg);
    }
    printf("cap %d len %d\n", actor_mq_cap(mq), actor_mq_length(mq));
    cr_expect_eq(actor_mq_length(mq), test_count[j], "second len not eq");
    for (int i = 0; i < test_count[j]; i++) {
      actor_mq_pop(mq, &tmp);
      msg.session = 0xAA + i;
      cr_expect_eq(msg.session, tmp.session, "second push pop not eq %X %X",
                   msg.session, tmp.session);
    }
    cr_expect_eq(actor_mq_length(mq), 0, "second len not eq");
  }
  actor_mq_release(mq);
  actor_globalmq_deinit();
}