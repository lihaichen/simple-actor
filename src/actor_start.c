#include <pthread.h>
#include <unistd.h>
#include "actor_common.h"
#include "actor_mq.h"
#include "actor_server.h"

static ACTOR_LOCK_TYPE mutex;

static void* thread_worker(void* p) {
  struct actor_message_queue* q = NULL;
  while (1) {
    q = actor_context_message_dispatch(q, 2);
    if (q == NULL) {
      ACTOR_ENTER_LOCK(&mutex);
      sleep(1);
      ACTOR_EXIT_LOCK(&mutex);
    }
  }
  return NULL;
}

void actor_start(int thread_count) {
  pthread_t pid[thread_count];
  ACTOR_PRINT("simple actor start...\n");
  ACTOR_INIT_LOCK(&mutex);
  actor_globalmq_init();
  actor_server_init();
  for (int i = 0; i < thread_count; i++) {
    pthread_create(&pid[i], NULL, thread_worker, NULL);
  }
  // for (int i = 0; i < thread_count; i++) {
  //   pthread_join(pid[i], NULL);
  // }
  ACTOR_PRINT("simple actor start5\n");
}
