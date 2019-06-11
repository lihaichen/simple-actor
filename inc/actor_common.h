#ifndef __ACTOR_COMMON_H__
#define __ACTOR_COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#include "actor_def.h"

#define acontainer_of(node, type, member) \
  ((type*)((char*)(node) - (unsigned long)(&((type*)0)->member)))

void actor_start(int thread_count);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
