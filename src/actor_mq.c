#include "actor_mq.h"
#include "actor_common.h"
#include "actor_list.h"
#include "actor_spinlock.h"

#define DEFAULT_QUEUE_SIZE 10
#define MAX_GLOBAL_MQ 100

struct actor_message_queue {
  struct actor_spinlock lock;
  int cap;
  int head;
  int tail;
  int release;
  int in_global;
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

void actor_globalmq_push(struct actor_message_queue* mq) {
  ACTOR_SPIN_LOCK(Q);
  alist_insert_after(&Q->list, &mq->list);
  ACTOR_SPIN_UNLOCK(Q);
}

struct actor_message_queue* actor_globalmq_pop() {
  struct actor_global_queue* q = Q;
  ACTOR_SPIN_LOCK(q);
  struct actor_message_queue* mq = NULL;
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
  q->in_global = 0;
  q->queue = ACTOR_MALLOC(sizeof(struct actor_message) * q->cap);
  alist_init(&q->list);
  return q;
}

struct actor_context* actor_mq_get_context(
    struct actor_message_queue* mq) {
  return mq->context;
}

int actor_mq_pop(struct actor_message_queue* mq,
                 struct actor_message* message) {
  int ret = 1;
  ACTOR_SPIN_LOCK(mq);
  if (mq->head != mq->tail) {
    *message = mq->queue[mq->head++];
    ret = 0;
    // ++mq->head %= mq->cap;
    if (mq->head >= mq->cap)
      mq->head = 0;
  }
  if (ret) {
    mq->in_global = 0;
  }
  ACTOR_SPIN_UNLOCK(mq);
  return ret;
}

static int expand_queue(struct actor_message_queue* mq) {
  if (mq->cap >= MAX_GLOBAL_MQ)
    return -1;

  struct actor_message* new_queue =
      ACTOR_MALLOC(sizeof(struct actor_message) * mq->cap * 2);
  if (new_queue == NULL)
    return -1;
  for (int i = 0; i < mq->cap; i++) {
    new_queue[i] = mq->queue[(mq->head + i) % mq->cap];
  }
  mq->head = 0;
  mq->tail = mq->cap;
  mq->cap *= 2;

  ACTOR_FREE(mq->queue);
  mq->queue = new_queue;
  return 0;
}

void actor_mq_push(struct actor_message_queue* mq,
                   struct actor_message* message) {
  ACTOR_SPIN_LOCK(mq);

  mq->queue[mq->tail] = *message;
  if (++mq->tail >= mq->cap) {
    mq->tail = 0;
  }

  if (mq->head == mq->tail) {
    if (expand_queue(mq) == -1) {
      ACTOR_PRINT("mq full, drap old msg\n");
      // 需要根据消息的情况进行释放。
      if (++mq->head >= mq->cap) {
        mq->tail = 0;
      }
    }
  }

  if (mq->in_global == 0) {
    mq->in_global = 1;
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
