#include <criterion/criterion.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "actor_heap.h"

#define HEAP_SIZE 20

static int max_heap(void* a, void* b) {
  return (int)(long)a > (int)(long)b ? 1 : -1;
}

Test(actor_aheap, sort) {
  struct aheap* heap = aheap_create(HEAP_SIZE, max_heap);
  time_t t = time(NULL);
  srand(t);
  cr_expect_eq(aheap_size(heap), HEAP_SIZE, "aheap cap error");
  cr_assert_not_null(heap, "aheap_create null");
  for (int i = 0; i < HEAP_SIZE; i++) {
    int res = aheap_insert(heap, (void*)(long)(rand() % 100));
    cr_expect_eq(res, 0, "aheap_insert error");
  }
  cr_expect_eq(aheap_len(heap), HEAP_SIZE, "aheap cap error");
  int ov = 100, nv = 0;
  for (int i = 0; i < HEAP_SIZE; i++) {
    nv = (int)(long)delFist(heap);
    cr_expect_geq(ov, nv, "heap sort error");
    ov = nv;
    printf("%d  ", nv);
  }
  printf("\n");
  aheap_free(heap);
}