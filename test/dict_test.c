#include <criterion/criterion.h>
#include <stdio.h>
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

actor_dict_type_t type1 = {.hash = hash_int,
                           .key_compare = NULL,
                           .key_dup = NULL,
                           .val_dup = NULL,
                           .key_delete = NULL,
                           .val_delete = NULL};

Test(actor_dict, dict_simple) {
  actor_dict_t* dict = actor_dict_create(&type1);
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

Test(actor_dict, dict_size) {
  actor_dict_t* dict = actor_dict_create(&type1);
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
