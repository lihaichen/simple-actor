#ifndef __ACTOR_HEAP_H__
#define __ACTOR_HEAP_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
#include "actor_def.h"

struct aheap;
// (a > b 1) (a == b 0) (a < b -1) max heap
typedef int (*compare)(void* a, void* b);
struct aheap {
  int len;
  int cap;
  // void * any data
  void** pq;
  compare compare;
};

inline struct aheap* aheap_create(int cap, compare c) {
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

inline void aheap_free(struct aheap* heap) {
  ACTOR_FREE(heap->pq);
  ACTOR_FREE(heap);
}

inline int aheap_init(struct aheap* heap, int cap, compare c) {
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
inline int aheap_destroy(struct aheap* heap) {
  ACTOR_FREE(heap->pq);
  return 0;
}

inline void aexch(struct aheap* heap, int i, int j) {
  void* tmp = heap->pq[i];
  heap->pq[i] = heap->pq[j];
  heap->pq[j] = tmp;
}

inline void aswim(struct aheap* heap, int k) {
  while ((k > 1) && (heap->compare(heap->pq[k / 2], heap->pq[k]) < 0)) {
    aexch(heap, k / 2, k);
    k /= 2;
  }
}

inline void asink(struct aheap* heap, int k) {
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

inline int aheap_isEmpty(struct aheap* heap) {
  return heap->len == 0;
}

inline int aheap_size(struct aheap* heap) {
  return heap->cap;
}

inline int aheap_len(struct aheap* heap) {
  return heap->len;
}

inline int aheap_insert(struct aheap* heap, void* v) {
  if (heap->len >= heap->cap)
    return -1;
  heap->pq[++heap->len] = v;
  aswim(heap, heap->len);
  return 0;
}

inline void* delFist(struct aheap* heap) {
  void* tmp = heap->pq[1];
  aexch(heap, 1, heap->len--);
  heap->pq[heap->len + 1] = NULL;
  asink(heap, 1);
  return tmp;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif