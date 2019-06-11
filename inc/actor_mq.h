#ifndef __ACTOR_MQ_H__
#define __ACTOR_MQ_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct actor_context;

struct actor_message {
  void* source;
  int session;
  int type;
  void* data;
  int sz;
};

#define ACTOR_MESSAGE_TYPE_MASK (SIZE_MAX >> 8)
#define ACTOR_MESSAGE_TYPE_SHIFT ((sizeof(size_t) - 1) * 8)

struct actor_message_queue;

void actor_globalmq_init(void);
void actor_globalmq_deinit(void);

void actor_globalmq_push(struct actor_message_queue* queue);
struct actor_message_queue* actor_globalmq_pop(void);
void actor_globalmq_rm(struct actor_message_queue* mq);

struct actor_message_queue* actor_mq_create(struct actor_context* context);
/**
 * @brief 释放mq
 * 释放前需要释放mq中msg,移除globalmq
 */
void actor_mq_release(struct actor_message_queue* mq);

struct actor_context* actor_mq_get_context(struct actor_message_queue* mq);
int actor_mq_pop(struct actor_message_queue* mq, struct actor_message* message);
void actor_mq_push(struct actor_message_queue* mq,
                   struct actor_message* message);
int actor_mq_length(struct actor_message_queue* mq);
int actor_mq_cap(struct actor_message_queue* mq);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
