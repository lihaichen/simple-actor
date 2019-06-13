#ifndef __ACTOR_DEF_H__
#define __ACTOR_DEF_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <semaphore.h>

#define ACTOR_MALLOC(size) malloc(size)
#define ACTOR_CALLOC(n, size) calloc(n, size)
#define ACTOR_FREE(p) free(p)

#define ACTOR_MSLEEP(ms) usleep((ms)*1000)

typedef unsigned long long actor_tick_t;
#define ACTOR_MAX_TICK 0xFFFFFFFFFFFFFFFF

#define ACTOR_GET_TICK(ms)                                                  \
  do {                                                                       \
    struct timespec spec;                                                    \
    clock_gettime(CLOCK_REALTIME, &spec);                                    \
    *ms = (spec.tv_nsec / 1000000 + (unsigned long long)spec.tv_sec * 1000); \
  } while (0)

#define ACTOR_PRINT(...) printf(__VA_ARGS__)

#define ACTOR_LOCK_TYPE pthread_mutex_t
#define ACTOR_INIT_LOCK(lock) pthread_mutex_init(lock, NULL)
#define ACTOR_DEL_LOCK(lock) pthread_mutex_destroy(lock)
#define ACTOR_ENTER_LOCK(lock) pthread_mutex_lock(lock)
#define ACTOR_TRY_ENTER_LOCK(lock) pthread_mutex_trylock(lock)
#define ACTOR_EXIT_LOCK(lock) pthread_mutex_unlock(lock)

#define ACTOR_SEM_TYPE sem_t
#define ACTOR_SEM_INIT(sem, p, v) sem_init(sem, p, v)
#define ACTOR_SEM_WAIT(sem) sem_wait(sem)
#define ACTOR_SEM_WAIT_TIME(sem, ts) sem_timedwait(sem, ts)
#define ACTOR_SME_DESTROY(sem) sem_destroy(sem)
#define ACTOR_SEM_POST(sem) sem_post(sem)
#define ACTOR_SEM_GET_VALUE(sem, value) sem_getvalue(sem, value)
#define ACTOR_SEM_SET_VALUE(sem, value)

#define ACTOR_ASSERT(EX)                                    \
  do {                                                       \
    if (!(EX)) {                                             \
      ACTOR_PRINT("%s,%s,%d", #EX, __FUNCTION__, __LINE__); \
    }                                                        \
  } while (0)

#define ATOM_CAS(ptr, oval, nval) __sync_bool_compare_and_swap(ptr, oval, nval)
#define ATOM_CAS_POINTER(ptr, oval, nval) __sync_bool_compare_and_swap(ptr, oval, nval)
#define ATOM_INC(ptr) __sync_add_and_fetch(ptr, 1)
#define ATOM_FINC(ptr) __sync_fetch_and_add(ptr, 1)
#define ATOM_DEC(ptr) __sync_sub_and_fetch(ptr, 1)
#define ATOM_FDEC(ptr) __sync_fetch_and_sub(ptr, 1)
#define ATOM_ADD(ptr,n) __sync_add_and_fetch(ptr, n)
#define ATOM_SUB(ptr,n) __sync_sub_and_fetch(ptr, n)
#define ATOM_AND(ptr,n) __sync_and_and_fetch(ptr, n)

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
