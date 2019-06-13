#ifndef __ACTOR_SERVER_H__
#define __ACTOR_SERVER_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct actor_context;
struct actor_message_queue;
struct actor_message;

#define ACTOR_MSG_TYPE_TEXT 0
#define ACTOR_MSG_TYPE_BIN 1
#define ACTOR_MSG_TYPE_RESPONSE 2
#define ACTOR_MSG_TYPE_INIT 3
#define ACTOR_MSG_TYPE_EXIT 4
#define ACTOR_MSG_TYPE_SYSTEM 5
#define ACTOR_MSG_TYPE_ERROR 6

#define ACTOR_MSG_TAG_DONTCOPY 0x10000
#define ACTOR_MSG_TAG_ALLOCSESSION 0x20000
#define ACTOR_MSG_TAG_NEEDRESPONSE 0x40000

typedef int (*actor_cb)(struct actor_context* context,
                        void* ud,
                        int type,
                        int session,
                        void* source,
                        const void* msg,
                        int sz);

extern int actor_context_total(void);
extern int actor_server_init(void);
extern void actor_context_grab(struct actor_context* ctx);
extern struct actor_context* actor_context_release(struct actor_context* ctx);
extern struct actor_context* actor_context_new(const char* name,
                                               const char* param);
extern struct actor_context* actor_context_find(const char* name);

extern void actor_context_callback(struct actor_context* context,
                                   actor_cb cb,
                                   void* ud);
extern char* actor_context_name(struct actor_context* context);

extern struct actor_message_queue* actor_context_message_dispatch(
    struct actor_message_queue* mq,
    int weight);

extern int actor_context_send(void* source,
                              void* destination,
                              int type,
                              int session,
                              void* data,
                              int sz);

extern void actor_destroy_message(struct actor_message* msg);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
