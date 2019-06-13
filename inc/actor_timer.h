#ifndef __ACTOR_TIMER_H__
#define __ACTOR_TIMER_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
#include "actor_def.h"
struct actor_context;

#define ACTOR_TIMER_FLAG_ONESHOT (0 << 0)
#define ACTOR_TIMER_FLAG_PERIOD (1 << 1)

void actor_timer_init(void);
int actor_timer_add(struct actor_context* context,
                     int session,
                     int time,
                     int flag);
int actor_timer_delete(struct actor_context* context, int session);
int actor_timer_restart(struct actor_context* context, int session);
void actor_timer_deinit(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif