#ifndef __ACTOR_HEAP_H__
#define __ACTOR_HEAP_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
#include "actor_def.h"

struct aheap;
// (a > b 1) (a == b 0) (a < b -1) max heap
typedef int (*compare)(void* a, void* b);
// 0 继续遍历 其他不在遍历
typedef int (*map)(int, void*);
struct aheap {
  int len;
  int cap;
  // void * any data
  void** pq;
  compare compare;
};

static inline struct aheap* aheap_create(int cap, compare c) {
  struct aheap* heap = (struct aheap*)ACTOR_MALLOC(sizeof(*heap));
  if (heap == NULL)
    return heap;
  heap->cap = cap;
  heap->len = 0;
  heap->compare = c;
  heap->pq = (void**)ACTOR_MALLOC(sizeof(void*) * (cap + 1));

  if (heap->pq == NULL) {
    ACTOR_FREE(heap);
    return NULL;
  }
  for (int i = 0; i < heap->cap; i++) {
    heap->pq[i] = NULL;
  }
  return heap;
}

static inline void aheap_free(struct aheap* heap) {
  ACTOR_FREE(heap->pq);
  ACTOR_FREE(heap);
}

static inline int aheap_init(struct aheap* heap, int cap, compare c) {
  heap->cap = cap;
  heap->len = 0;
  heap->compare = c;
  heap->pq = (void**)ACTOR_MALLOC(sizeof(void*) * (cap + 1));

  if (heap->pq == NULL) {
    return -1;
  }
  for (int i = 0; i < heap->cap; i++) {
    heap->pq[i] = NULL;
  }
  return 0;
}

static inline int aheap_destroy(struct aheap* heap) {
  ACTOR_FREE(heap->pq);
  return 0;
}

static inline void aexch(struct aheap* heap, int i, int j) {
  void* tmp = heap->pq[i];
  heap->pq[i] = heap->pq[j];
  heap->pq[j] = tmp;
}

static inline void aswim(struct aheap* heap, int k) {
  while ((k > 1) && (heap->compare(heap->pq[k / 2], heap->pq[k]) < 0)) {
    aexch(heap, k / 2, k);
    k /= 2;
  }
}

static inline void asink(struct aheap* heap, int k) {
  while (2 * k <= heap->len) {
    int j = 2 * k;
    if (j < heap->len && heap->compare(heap->pq[j], heap->pq[j + 1]) < 0)
      j++;
    if (heap->compare(heap->pq[k], heap->pq[j]) > 0)
      break;
    aexch(heap, k, j);
    k = j;
  }
}

static inline int aheap_isEmpty(struct aheap* heap) {
  return heap->len == 0;
}

static inline int aheap_size(struct aheap* heap) {
  return heap->cap;
}

static inline int aheap_len(struct aheap* heap) {
  return heap->len;
}

static inline int aheap_insert(struct aheap* heap, void* v) {
  if (heap->len >= heap->cap)
    return -1;
  heap->pq[++heap->len] = v;
  aswim(heap, heap->len);
  return 0;
}

static inline void* aheap_get(struct aheap* heap, int index) {
  if (index < 1 || index > heap->len) {
    return NULL;
  }
  return heap->pq[index];
}

static inline void* aheap_getFist(struct aheap* heap) {
  return heap->pq[1];
}

static inline void* aheap_delFist(struct aheap* heap) {
  void* tmp = heap->pq[1];
  aexch(heap, 1, heap->len--);
  heap->pq[heap->len + 1] = NULL;
  asink(heap, 1);
  return tmp;
}

static inline void* aheap_delete(struct aheap* heap, int k) {
  if (k < 1 || k > heap->len) {
    return NULL;
  }
  void* tmp = heap->pq[k];
  aexch(heap, k, heap->len--);
  asink(heap, k);
  asink(heap, k);
  heap->pq[heap->len + 1] = NULL;
  return tmp;
}

static inline void aheap_foreach(struct aheap* heap, map func) {
  for (int i = 1; i <= heap->len; i++) {
    if (func(i, heap->pq[i]) != 0)
      break;
  }
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif