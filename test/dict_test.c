#include <criterion/criterion.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "actor_dict.h"

#define HASH_SIZE 200

static unsigned int hash_int(const void* key) {
  char buf[sizeof(int)] = {0};
  unsigned long key_v = (unsigned long)key;
  for (int i = 0; i < sizeof(int); i++) {
    buf[i] = (key_v >> (8 * i)) & 0xFF;
  }
  return murmurhash(buf, sizeof(int), 0);
}

static actor_dict_type_t type_int = {.hash = hash_int,
                                     .key_compare = NULL,
                                     .key_dup = NULL,
                                     .val_dup = NULL,
                                     .key_delete = NULL,
                                     .val_delete = NULL};

static unsigned int hash_string(const void* key) {
  return murmurhash(key, strlen(key), 0);
}

static int key_compare_string(const void* key1, const void* key2) {
  return strcmp(key1, key2) == 0 ? 1 : 0;
}

static void* key_dup_string(const void* key) {
  int len = strlen(key) + 1;
  void* res = malloc(len);
  memset(res, 0, len);
  strcpy(res, key);
  return res;
}

static void* val_dup_string(const void* val) {
  int len = strlen(val) + 1;
  void* res = malloc(len);
  memset(res, 0, len);
  strcpy(res, val);
  return res;
}

static void key_delete_string(void* key) {
  free(key);
}

static void val_delete_string(void* val) {
  free(val);
}

static actor_dict_type_t type_string = {.hash = hash_string,
                                        .key_compare = key_compare_string,
                                        .key_dup = key_dup_string,
                                        .val_dup = val_dup_string,
                                        .key_delete = key_delete_string,
                                        .val_delete = val_delete_string};

Test(actor_dict, dict_int_simple) {
  actor_dict_t* dict = actor_dict_create(&type_int);
  cr_expect_not_null(dict, "actor_dict_create null");
  long key = 0x2;
  int ret = actor_dict_add(dict, (void*)key, NULL);
  cr_expect_eq(0, ret, "dict add error");
  ret = actor_dict_add(dict, (void*)key, NULL);
  cr_expect_neq(0, ret, "dict add again error");

  actor_dict_entry_t* entry = actor_dict_find(dict, (void*)key);
  cr_expect_not_null(entry, "actor_dict_find null");
  cr_expect_eq(key, entry->key, "dict add error");
  ret = actor_dict_delete(dict, (void*)key);
  cr_expect_eq(0, ret, "dict delete error");
  entry = actor_dict_find(dict, (void*)key);
  cr_expect_null(entry, "actor_dict_find null");
  actor_dict_destroy(dict);
  return;
}

Test(actor_dict, dict_string_size) {
  actor_dict_t* dict = actor_dict_create(&type_string);
  cr_expect_not_null(dict, "actor_dict_create null");
  char buf[64] = {0};

  for (int i = 0; i < HASH_SIZE; i++) {
    memset(buf, 0, sizeof(buf));
    sprintf(buf, "hello%03d", i);
    actor_dict_add(dict, (void*)buf, buf);
  }
  cr_expect_eq(HASH_SIZE, dict->ht[0].used, "used not match");
  for (int i = 0; i < HASH_SIZE; i++) {
    memset(buf, 0, sizeof(buf));
    sprintf(buf, "hello%03d", i);
    actor_dict_entry_t* entry = actor_dict_find(dict, (void*)buf);
    cr_expect_not_null(entry, "actor_dict_find null");
    cr_expect_str_eq(buf, entry->key, "actor_dict_find key not eq");
    actor_dict_delete(dict, (void*)buf);
  }
  cr_expect_eq(0, dict->ht[0].used, "used not match");
  for (int i = 0; i < HASH_SIZE; i++) {
    memset(buf, 0, sizeof(buf));
    sprintf(buf, "hello%03d", i);
    actor_dict_entry_t* entry = actor_dict_find(dict, (void*)buf);
    cr_expect_null(entry, "actor_dict_find must null");
  }
  actor_dict_destroy(dict);
  return;
}

Test(actor_dict, dict_size) {
  actor_dict_t* dict = actor_dict_create(&type_int);
  cr_expect_not_null(dict, "actor_dict_create null");
  time_t t = time(NULL);
  srand(t);
  long* buf = malloc(sizeof(long) * HASH_SIZE);

  for (int i = 0; i < HASH_SIZE; i++) {
    buf[i] = rand() & 0xFFFFFFFF;
  }
  for (int i = 0; i < HASH_SIZE; i++) {
    actor_dict_add(dict, (void*)buf[i], NULL);
  }
  for (int i = 0; i < HASH_SIZE; i++) {
    actor_dict_entry_t* entry = actor_dict_find(dict, (void*)buf[i]);
    cr_expect_not_null(entry, "actor_dict_find null");
    cr_expect_eq(buf[i], entry->key, "actor_dict_find key not eq");
    actor_dict_delete(dict, (void*)buf[i]);
  }

  for (int i = 0; i < HASH_SIZE; i++) {
    actor_dict_entry_t* entry = actor_dict_find(dict, (void*)buf[i]);
    cr_expect_null(entry, "actor_dict_find must null");
  }

  free(buf);
  actor_dict_destroy(dict);
  return;
}

Test(actor_dict, iterator) {
  actor_dict_t* dict = actor_dict_create(&type_int);
  cr_expect_not_null(dict, "actor_dict_create null");
  long buf[] = {0x1, 0x2, 0x2, 0x3, 0x2, 0x5, 0x3, 0x1, 0x4, 0x5, 0x1, 0x3};
  for (int i = 0; i < sizeof(buf) / sizeof(long); i++) {
    actor_dict_add(dict, (void*)buf[i], NULL);
  }
  actor_dict_interator_t* iter = actor_dict_create_iterator(dict);
  cr_expect_not_null(iter, "actor_dict_create_iterator null");
  actor_dict_entry_t* entry = actor_dict_iterator_next(iter);
  while (entry != NULL) {
    printf("key %lX\n", (long)entry->key);
    entry = actor_dict_iterator_next(iter);
  }

  actor_dict_destroy_iterator(iter);
  actor_dict_destroy(dict);
}