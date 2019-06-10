#ifndef __ACTOR_SERVER_H__
#define __ACTOR_SERVER_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct actor_context;
typedef int (*actor_cb)(struct actor_context * context, void *ud,
    int type, int session, int source , const void * msg, int sz);

extern int actor_context_total(void);
extern int actor_server_init(void);
extern void actor_context_grab(struct actor_context* ctx);
extern struct actor_context* actor_context_release(struct actor_context* ctx);
extern struct actor_context* actor_context_new(const char* name, const char* param);
extern void actor_callback(struct actor_context* context, void* ud, actor_cb cb);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

