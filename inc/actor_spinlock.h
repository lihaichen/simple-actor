#ifndef __ACTOR_SPINLOCK_H__
#define __ACTOR_SPINLOCK_H__
#include "actor_def.h"
#define ACTOR_SPIN_INIT(q) actor_spinlock_init(&(q)->lock);
#define ACTOR_SPIN_LOCK(q) actor_spinlock_lock(&(q)->lock);
#define ACTOR_SPIN_UNLOCK(q) actor_spinlock_unlock(&(q)->lock);
#define ACTOR_SPIN_DESTROY(q) actor_spinlock_destroy(&(q)->lock);

#ifndef USE_PTHREAD_LOCK

struct actor_spinlock {
  int lock;
};

static inline void actor_spinlock_init(struct actor_spinlock* lock) {
  lock->lock = 0;
}

static inline void actor_spinlock_lock(struct actor_spinlock* lock) {
  while (__sync_lock_test_and_set(&lock->lock, 1)) {
  }
}

static inline int actor_spinlock_trylock(struct actor_spinlock* lock) {
  return __sync_lock_test_and_set(&lock->lock, 1) == 0;
}

static inline void actor_spinlock_unlock(struct actor_spinlock* lock) {
  __sync_lock_release(&lock->lock);
}

static inline void actor_spinlock_destroy(struct actor_spinlock* lock) {
  (void)lock;
}

#else

#include <pthread.h>

struct actor_spinlock {
  ACTOR_LOCK_TYPE lock;
};

static inline void actor_spinlock_init(struct actor_spinlock* lock) {
  ACTOR_INIT_LOCK(&lock->lock);
}

static inline void actor_spinlock_lock(struct actor_spinlock* lock) {
  ACTOR_ENTER_LOCK(&lock->lock);
}

static inline int actor_spinlock_trylock(struct actor_spinlock* lock) {
  return ACTOR_TRY_ENTER_LOCK(&lock->lock) == 0;
}

static inline void actor_spinlock_unlock(struct actor_spinlock* lock) {
  ACTOR_EXIT_LOCK(&lock->lock);
}

static inline void actor_spinlock_destroy(struct actor_spinlock* lock) {
  ACTOR_DEL_LOCK(&lock->lock);
}

#endif

#endif
