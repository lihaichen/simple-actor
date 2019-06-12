#include "actor_server.h"
#include "actor_common.h"
#include "actor_list.h"
#include "actor_mq.h"
#include "actor_spinlock.h"

#define ACTOR_NAME_SIZE 16

struct actor_context {
  void* cb_ud;
  actor_cb cb;
  struct actor_message_queue* queue;
  unsigned int cpu_cost;   // in microsec
  unsigned int cpu_start;  // in microsec
  int session;
  int ref;
  char name[ACTOR_NAME_SIZE];
  alist_node_t list;
};

struct actor_node {
  struct actor_spinlock lock;
  int total;
  alist_node_t list;
};

static struct actor_node G_NODE;
static void context_inc(void);
static void context_dec(void);
static void dispatch_message(struct actor_context* ctx,
                             struct actor_message* msg);

int actor_context_total() {
  return G_NODE.total;
}

static void context_inc() {
  ATOM_INC(&G_NODE.total);
}

static void context_dec() {
  ATOM_DEC(&G_NODE.total);
}

int actor_server_init() {
  G_NODE.total = 0;
  alist_init(&G_NODE.list);
  ACTOR_SPIN_INIT(&G_NODE);
  return 0;
}

void actor_context_grab(struct actor_context* ctx) {
  ATOM_INC(&ctx->ref);
}

struct actor_context* actor_context_release(struct actor_context* ctx) {
  ATOM_DEC(&ctx->ref);
  if (ctx->ref == 0) {
    // 消息队列的消息处理掉
    while (1) {
      struct actor_message msg;
      int ret = actor_mq_pop(ctx->queue, &msg);
      if (ret != 0)
        break;
      actor_destroy_message(&msg);
    }
    alist_remove(&ctx->list);
    ACTOR_FREE(ctx->queue);
    ACTOR_FREE(ctx);
    context_dec();
    return NULL;
  }
  return ctx;
}

struct actor_context* actor_context_new(const char* name, const char* param) {
  struct actor_context* ctx = ACTOR_CALLOC(1, sizeof(*ctx));
  ACTOR_ASSERT(ctx != NULL);
  ctx->ref = 2;
  ctx->session = 0;
  ctx->cb_ud = NULL;
  ctx->cb = NULL;
  ctx->cpu_cost = 0;
  ctx->cpu_start = 0;
  strncpy(ctx->name, name, ACTOR_NAME_SIZE - 1);
  ctx->queue = actor_mq_create(ctx);
  ACTOR_ASSERT(ctx->queue != NULL);
  alist_init(&ctx->list);
  ACTOR_SPIN_LOCK(&G_NODE);
  alist_insert_after(&G_NODE.list, &ctx->list);
  ACTOR_SPIN_UNLOCK(&G_NODE);
  context_inc();
  actor_context_release(ctx);
  return ctx;
}

void actor_context_callback(struct actor_context* context,
                            actor_cb cb,
                            void* ud) {
  context->cb = cb;
  context->cb_ud = ud;
}

char* actor_context_name(struct actor_context* context) {
  return context->name;
}

struct actor_context* actor_context_find(const char* name) {
  alist_node_t* list = &G_NODE.list;
  struct actor_context* res = NULL;
  for (struct alist_node* node = list->next; node != list; node = node->next) {
    res = acontainer_of(node, struct actor_context, list);
    if (strncmp(res->name, name, ACTOR_NAME_SIZE) == 0)
      return res;
  }
  return NULL;
}

struct actor_message_queue* actor_context_message_dispatch(
    struct actor_message_queue* mq,
    int weight) {
  if (mq == NULL) {
    mq = actor_globalmq_pop();
    if (mq == NULL)
      return mq;
  }
  struct actor_context* ctx = actor_mq_get_context(mq);
  int len = actor_mq_length(mq);
  len = len > weight ? weight : len;
  len = len < 1 ? 1 : len;
  struct actor_message msg;
  for (int i = 0; i < len; i++) {
    if (actor_mq_pop(mq, &msg)) {
      return actor_globalmq_pop();
    }
    if (ctx->cb != NULL) {
      dispatch_message(ctx, &msg);
    }
    // 按照规则释放msg
    actor_destroy_message(&msg);
  }
  struct actor_message_queue* nq = actor_globalmq_pop();
  if (nq) {
    actor_globalmq_push(mq);
    mq = nq;
  }
  return mq;
}
extern void actor_thread_wakeup(void);
int actor_context_send(void* source,
                       void* destination,
                       int type,
                       int session,
                       void* data,
                       int sz) {
  struct actor_message msg;
  // ACTOR_ASSERT(source != NULL);
  ACTOR_ASSERT(destination != NULL);
  msg.source = source;
  msg.session = session;

  msg.sz = sz;
  msg.type = type;
  if ((type & ACTOR_MSG_TAG_DONTCOPY) == ACTOR_MSG_TAG_DONTCOPY) {
    msg.data = data;
  } else {
    msg.data = ACTOR_MALLOC(sz + 1);
    ACTOR_ASSERT(msg.data != NULL);
    ((char*)msg.data)[sz] = 0;
    memcpy(msg.data, data, sz);
  }
  struct actor_context* des_ctx = (struct actor_context*)(destination);
  actor_mq_push(des_ctx->queue, &msg);
  actor_thread_wakeup();
  return session;
}

void actor_destroy_message(struct actor_message* msg) {
  if ((msg->type & ACTOR_MSG_TAG_DONTCOPY) != ACTOR_MSG_TAG_DONTCOPY) {
    ACTOR_FREE(msg->data);
  }
  return;
}

static void dispatch_message(struct actor_context* ctx,
                             struct actor_message* msg) {
  ctx->cb(ctx, ctx->cb_ud, msg->type, msg->session, msg->source, msg->data,
          msg->sz);
}
