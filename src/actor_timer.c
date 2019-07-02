#include <pthread.h>
#include <unistd.h>
#include "actor_common.h"
#include "actor_def.h"
#include "actor_spinlock.h"

typedef struct actor_timer {
  int session;
  struct actor_context* context;
  actor_tick_t expire;
  int time;  // <0 as oneshot timer, >0 as period timer
} actor_timer_t;

typedef struct actor_timer_node {
  struct aheap heap;
  struct actor_spinlock lock;
  pthread_t pid;
  ACTOR_SEM_TYPE sem;
  int quit;
} actor_timer_node_t;

static int min_heap(void* a, void* b);
// 内部含锁
static int insert_timer(actor_timer_t* timer);
// 需要锁保护
static int find_timer(struct actor_context* context, int session);

static void* thread_worker(void* p);

static actor_timer_node_t G_NODE;

void actor_timer_init(void) {
  ACTOR_SPIN_INIT(&G_NODE);
  ACTOR_SEM_INIT(&G_NODE.sem, 0, 0);
  aheap_init(&G_NODE.heap, 16, min_heap);
  pthread_create(&G_NODE.pid, NULL, thread_worker, NULL);
  return;
}

void actor_timer_deinit(void) {
  actor_timer_t* timer = NULL;
  G_NODE.quit = 1;
  pthread_join(G_NODE.pid, NULL);
  ACTOR_SPIN_LOCK(&G_NODE);
  do {
    timer = aheap_delFist(&G_NODE.heap);
    ACTOR_FREE(timer);
  } while (timer != NULL);
  ACTOR_SPIN_UNLOCK(&G_NODE);
  aheap_destroy(&G_NODE.heap);
  ACTOR_SME_DESTROY(&G_NODE.sem);
  ACTOR_SPIN_DESTROY(&G_NODE);
  return;
}

int actor_timer_add(struct actor_context* context,
                    int session,
                    int time,
                    int flag) {
  if (time == 0) {
    // 直接发送消息
    actor_context_send(context, context, ACTOR_MSG_TYPE_RESPONSE, session, NULL,
                       0);
    return 0;
  }
  actor_timer_t* timer = ACTOR_MALLOC(sizeof(*timer));
  ACTOR_ASSERT(timer != NULL);
  memset(timer, 0, sizeof(*timer));
  timer->session = session;
  timer->context = context;

  if (flag == ACTOR_TIMER_FLAG_PERIOD) {
    timer->time = time;
  } else {
    timer->time = -time;
  }
  insert_timer(timer);
  return 0;
}

int actor_timer_delete(struct actor_context* context, int session) {
  int index = 0, res = 0;
  actor_timer_t* timer = NULL;
  ACTOR_SPIN_LOCK(&G_NODE);
  index = find_timer(context, session);
  if (index < 1) {
    res = -1;
    goto breakout;
  }
  timer = aheap_delete(&G_NODE.heap, index);
breakout:
  ACTOR_SPIN_UNLOCK(&G_NODE);
  if (timer != NULL)
    ACTOR_FREE(timer);
  return res;
}

int actor_timer_restart(struct actor_context* context, int session) {
  int index = 0;
  ACTOR_SPIN_LOCK(&G_NODE);
  index = find_timer(context, session);
  if (index < 1) {
    ACTOR_SPIN_UNLOCK(&G_NODE);
    return -1;
  }
  actor_timer_t* timer = aheap_delete(&G_NODE.heap, index);
  ACTOR_SPIN_UNLOCK(&G_NODE);
  insert_timer(timer);
  return 0;
}

static int min_heap(void* a, void* b) {
  actor_timer_t *timer_a, *timer_b;
  timer_a = (actor_timer_t*)a;
  timer_b = (actor_timer_t*)b;

  return timer_a->expire > timer_b->expire ? -1 : 1;
}

static int insert_timer(actor_timer_t* timer) {
  actor_tick_t current_ms;
  int res = 0;
  ACTOR_GET_TICK(&current_ms);
  if (timer->time < 0) {
    timer->expire = -timer->time + current_ms;
  } else {
    timer->expire = timer->time + current_ms;
  }

  ACTOR_SPIN_LOCK(&G_NODE);
  res = aheap_insert(&G_NODE.heap, timer);
  ACTOR_SPIN_UNLOCK(&G_NODE);
  // wake up timer thread
  int sem_value = 0;
  ACTOR_SEM_GET_VALUE(&G_NODE.sem, &sem_value);
  // printf("wake up timer %d\n", sem_value);
  if (sem_value <= 0) {
    ACTOR_SEM_POST(&G_NODE.sem);
    // printf("wait sem value ===%d\n", sem_value);
  }
  return res;
}

// 需要锁保护
static int find_timer(struct actor_context* context, int session) {
  actor_timer_t* timer = NULL;
  for (int i = 1; i <= aheap_len(&G_NODE.heap); i++) {
    timer = (actor_timer_t*)aheap_get(&G_NODE.heap, i);
    if (timer != NULL && timer->context == context &&
        timer->session == session) {
      return i;
    }
  }
  return 0;
}
static void* thread_worker(void* p) {
  ACTOR_PRINT("thread_timer_worker start\n");
  while (!G_NODE.quit) {
    actor_tick_t current_ms;
    actor_timer_t* timer = NULL;
    struct timespec ts;
    int diff = -1;
    while (1) {
      if (aheap_len(&G_NODE.heap) < 1)
        break;
      ACTOR_GET_TICK(&current_ms);
      timer = aheap_getFist(&G_NODE.heap);
      if (timer == NULL)
        break;
      diff = timer->expire - current_ms;
      // not timeout
      if (!IS_TIMEOUT(current_ms, timer->expire))
        break;
      ACTOR_SPIN_LOCK(&G_NODE);
      timer = aheap_delFist(&G_NODE.heap);
      ACTOR_SPIN_UNLOCK(&G_NODE);
      actor_context_send(timer->context, timer->context,
                         ACTOR_MSG_TYPE_RESPONSE, timer->session, NULL, 0);
      if (timer->time > 0) {
        insert_timer(timer);
      } else {
        ACTOR_FREE(timer);
      }
    }

    ACTOR_GET_TICK(&current_ms);
    if (diff < 0) {
      current_ms += 1000 * 10;  // delay 1 second
    } else {
      current_ms += diff;
    }
    ts.tv_nsec = (current_ms % 1000) * 1000 * 1000;
    ts.tv_sec = current_ms / 1000;
    ACTOR_SEM_WAIT_TIME(&G_NODE.sem, &ts);
  }
  ACTOR_PRINT("thread_timer_worker exit\n");
  return NULL;
}
