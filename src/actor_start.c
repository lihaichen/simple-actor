#include <pthread.h>
#include <unistd.h>
#include "actor_common.h"
#include "actor_def.h"
#include "actor_spinlock.h"

struct monitor {
  int count;
  int sleep;
  pthread_cond_t cond;
  ACTOR_LOCK_TYPE mutex;
  pthread_t* pid;
};

static struct monitor M;

// static ACTOR_LOCK_TYPE mutex;

void actor_thread_wakeup(void) {
  if (M.sleep > 0) {
    // ACTOR_PRINT("wakeup %d\n", M.sleep);
    pthread_cond_signal(&M.cond);
  }
}

static void* thread_worker(void* p) {
  struct actor_message_queue* q = NULL;
  while (1) {
    // ACTOR_PRINT("thread %ld\n", (long)p);
    q = actor_context_message_dispatch(q, 1);
    if (q == NULL) {
      if (ACTOR_ENTER_LOCK(&M.mutex) == 0) {
        M.sleep++;
        // ACTOR_PRINT("sleep count %d\n", M.sleep);
        pthread_cond_wait(&M.cond, &M.mutex);
        M.sleep--;
        ACTOR_EXIT_LOCK(&M.mutex);
      }
    }
  }
  return NULL;
}

void actor_start(int thread_count) {
  M.pid = ACTOR_MALLOC(sizeof(*M.pid) * thread_count);
  ACTOR_ASSERT(M.pid);
  ACTOR_INIT_LOCK(&M.mutex);
  pthread_cond_init(&M.cond, NULL);
  M.count = thread_count;
  M.sleep = 0;
  actor_globalmq_init();
  actor_server_init();
  actor_timer_init();
  for (int i = 0; i < thread_count; i++) {
    pthread_create(&M.pid[i], NULL, thread_worker, (void*)(long)i);
  }
  ACTOR_PRINT("simple actor start...\n");
}

void actor_stop() {
  for (int i = 0; i < M.count; i++) {
    pthread_cancel(M.pid[i]);
  }
  actor_timer_deinit();
  actor_server_deinit();
  actor_globalmq_deinit();
  pthread_cond_destroy(&M.cond);
  ACTOR_DEL_LOCK(&M.mutex);
  ACTOR_FREE(M.pid);
  ACTOR_PRINT("simple actor end...\n");
  return;
}
