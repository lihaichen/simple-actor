#include <pthread.h>
#include <unistd.h>
#include "actor_common.h"
#include "actor_mq.h"
#include "actor_server.h"

struct monitor {
  int count;
  int sleep;
  pthread_cond_t cond;
  pthread_mutex_t mutex;
};

static struct monitor M;

static ACTOR_LOCK_TYPE mutex;

void actor_thread_wakeup(void) {
  if (M.sleep > 0) {
    ACTOR_PRINT("wakeup %d\n", M.sleep);
    pthread_cond_signal(&M.cond);
  }
}

static void* thread_worker(void* p) {
  struct actor_message_queue* q = NULL;
  while (1) {
    ACTOR_PRINT("thread %d\n", (int)p);
    q = actor_context_message_dispatch(q, 1);
    if (q == NULL) {
      // ACTOR_ENTER_LOCK(&mutex);
      // sleep(1);
      // ACTOR_EXIT_LOCK(&mutex);
      if (pthread_mutex_lock(&M.mutex) == 0) {
        M.sleep++;
        ACTOR_PRINT("sleep count %d\n", M.sleep);
        pthread_cond_wait(&M.cond, &M.mutex);
        M.sleep--;
        pthread_mutex_unlock(&M.mutex);
      }
    }
  }
  return NULL;
}

void actor_start(int thread_count) {
  pthread_t pid[thread_count];
  ACTOR_PRINT("simple actor start...\n");
  pthread_mutex_init(&M.mutex, NULL);
  pthread_cond_init(&M.cond, NULL);
  M.count = thread_count;
  M.sleep = 0;
  // ACTOR_INIT_LOCK(&mutex);
  actor_globalmq_init();
  actor_server_init();
  for (int i = 0; i < thread_count; i++) {
    pthread_create(&pid[i], NULL, thread_worker, i);
  }
  // for (int i = 0; i < thread_count; i++) {
  //   pthread_join(pid[i], NULL);
  // }
  ACTOR_PRINT("simple actor start5\n");
}
