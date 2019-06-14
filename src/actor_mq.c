#include "actor_common.h"
#include "actor_def.h"
#include "actor_spinlock.h"

#define DEFAULT_QUEUE_SIZE 10
#define MAX_GLOBAL_MQ 100

struct actor_message_queue {
  struct actor_spinlock lock;
  int cap;
  int head;
  int tail;
  int release;
  struct actor_message* queue;
  alist_node_t list;
  struct actor_context* context;
};

struct actor_global_queue {
  alist_node_t list;
  struct actor_spinlock lock;
};

static struct actor_global_queue* Q = NULL;

void actor_globalmq_init(void) {
  struct actor_global_queue* q = ACTOR_MALLOC(sizeof(*q));
  memset(q, 0, sizeof(*q));
  alist_init(&q->list);
  ACTOR_SPIN_INIT(q);
  Q = q;
}

void actor_globalmq_deinit(void) {
  ACTOR_SPIN_DESTROY(Q);
  ACTOR_FREE(Q);
  Q = NULL;
}

void actor_globalmq_push(struct actor_message_queue* mq) {
  ACTOR_SPIN_LOCK(Q);
  alist_insert_before(&Q->list, &mq->list);
  ACTOR_SPIN_UNLOCK(Q);
}

void actor_globalmq_rm(struct actor_message_queue* mq) {
  ACTOR_SPIN_LOCK(Q);
  alist_remove(&mq->list);
  ACTOR_SPIN_UNLOCK(Q);
}

struct actor_message_queue* actor_globalmq_pop() {
  struct actor_global_queue* q = Q;
  struct actor_message_queue* mq = NULL;
  ACTOR_SPIN_LOCK(q);
  if (!alist_isempty(&Q->list)) {
    mq = acontainer_of(Q->list.next, struct actor_message_queue, list);
    alist_remove(&mq->list);
  }
  ACTOR_SPIN_UNLOCK(q);
  return mq;
}

struct actor_message_queue* actor_mq_create(struct actor_context* context) {
  struct actor_message_queue* q = ACTOR_MALLOC(sizeof(*q));
  q->cap = DEFAULT_QUEUE_SIZE;
  q->head = 0;
  q->tail = 0;
  q->context = context;
  ACTOR_SPIN_INIT(q);
  q->queue = ACTOR_MALLOC(sizeof(struct actor_message) * q->cap);
  alist_init(&q->list);
  return q;
}
/**
 * @brief 释放mq
 * 释放前需要释放mq中msg,移除globalmq
 */
void actor_mq_release(struct actor_message_queue* mq) {
  actor_globalmq_rm(mq);
  ACTOR_SPIN_DESTROY(mq);
  ACTOR_FREE(mq->queue);
  ACTOR_FREE(mq);
  return;
}

struct actor_context* actor_mq_get_context(struct actor_message_queue* mq) {
  return mq->context;
}

int actor_mq_pop(struct actor_message_queue* mq,
                 struct actor_message* message) {
  int ret = -1;
  ACTOR_SPIN_LOCK(mq);
  if (mq->head != mq->tail) {
    *message = mq->queue[mq->head++];
    ret = 0;
    // ++mq->head %= mq->cap;
    if (mq->head >= mq->cap)
      mq->head = 0;
  }
  ACTOR_SPIN_UNLOCK(mq);
  return ret;
}

static int expand_queue(struct actor_message_queue* mq) {
  if (mq->cap >= MAX_GLOBAL_MQ)
    return -1;

  struct actor_message* new_queue =
      ACTOR_MALLOC(sizeof(struct actor_message) * (mq->cap + 10));
  if (new_queue == NULL)
    return -1;
  // ACTOR_SPIN_LOCK(mq);
  for (int i = 0; i < mq->cap; i++) {
    new_queue[i] = mq->queue[(mq->head + i) % mq->cap];
  }
  mq->head = 0;
  mq->tail = mq->cap;
  mq->cap += 10;
  ACTOR_FREE(mq->queue);
  mq->queue = new_queue;
  // ACTOR_SPIN_UNLOCK(mq);
  return 0;
}

extern void actor_destroy_message(struct actor_message* msg);

void actor_mq_push(struct actor_message_queue* mq,
                   struct actor_message* message) {
  ACTOR_SPIN_LOCK(mq);

  mq->queue[mq->tail] = *message;
  if (++mq->tail >= mq->cap) {
    mq->tail = 0;
  }

  if (mq->head == mq->tail) {
    if (expand_queue(mq) != 0) {
      ACTOR_PRINT("mq full, drap old msg\n");
      // 需要根据消息的情况进行释放。
      actor_destroy_message(&mq->queue[mq->head]);
      if (++mq->head >= mq->cap) {
        mq->tail = 0;
      }
    }
  }
  if (alist_isempty(&mq->list)) {
    actor_globalmq_push(mq);
  }
  ACTOR_SPIN_UNLOCK(mq);
}

int actor_mq_length(struct actor_message_queue* mq) {
  int head, tail, cap;

  ACTOR_SPIN_LOCK(mq);
  head = mq->head;
  tail = mq->tail;
  cap = mq->cap;
  ACTOR_SPIN_UNLOCK(mq);

  if (head <= tail) {
    return tail - head;
  }
  return tail + cap - head;
}

int actor_mq_cap(struct actor_message_queue* mq) {
  return mq->cap;
}