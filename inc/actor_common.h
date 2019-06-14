#ifndef __ACTOR_COMMON_H__
#define __ACTOR_COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "actor_heap.h"
#include "actor_list.h"
#include "actor_mq.h"
#include "actor_server.h"
#include "actor_timer.h"

#define acontainer_of(node, type, member) \
  ((type*)((char*)(node) - (unsigned long)(&((type*)0)->member)))

void actor_start(int thread_count);
void actor_stop(void);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
