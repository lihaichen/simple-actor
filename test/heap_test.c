#include <criterion/criterion.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "actor_heap.h"

#define HEAP_SIZE 20

static int max_heap(void* a, void* b) {
  return (int)(long)a > (int)(long)b ? 1 : -1;
}

static int min_heap(void* a, void* b) {
  return (int)(long)a > (int)(long)b ? -1 : 1;
}

static int print_heap(int k, void* v) {
  printf("%02ld ", (long)v);
  return 0;
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
  aheap_foreach(heap, print_heap);
  printf("\n");
  int ov = 100, nv = 0;
  for (int i = 0; i < HEAP_SIZE; i++) {
    nv = (int)(long)aheap_delFist(heap);
    cr_expect_geq(ov, nv, "heap sort error");
    ov = nv;
    printf("%02d ", nv);
  }
  printf("\n");
  aheap_free(heap);
}

Test(actor_aheap, delete) {
  struct aheap* heap = aheap_create(HEAP_SIZE, min_heap);
  time_t t = time(NULL);
  srand(t);
  cr_expect_eq(aheap_size(heap), HEAP_SIZE, "aheap cap error");
  cr_assert_not_null(heap, "aheap_create null");
  for (int i = 0; i < HEAP_SIZE; i++) {
    int res = aheap_insert(heap, (void*)(long)(rand() % 100));
    cr_expect_eq(res, 0, "aheap_insert error");
  }
  cr_expect_eq(aheap_len(heap), HEAP_SIZE, "aheap cap error");
  aheap_foreach(heap, print_heap);
  printf("\n");
  for (int i = 0; i < HEAP_SIZE / 2; i++) {
    aheap_delete(heap, (rand() % heap->len) + 1);
  }

  int ov = 0, nv = 0;
  for (int i = 0; i < heap->len; i++) {
    nv = (int)(long)aheap_delFist(heap);
    cr_expect_leq(ov, nv, "heap sort error");
    ov = nv;
    printf("%02d ", nv);
  }
  printf("\n");
  aheap_free(heap);
}