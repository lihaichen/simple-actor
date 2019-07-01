#ifndef __ACTOR_DICT_H__
#define __ACTOR_DICT_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
#include "actor_common.h"

typedef unsigned int ACTOR_HASH_TYPE;

typedef struct actor_dict_entry {
  void* key;
  union dict {
    void* val;
    double d;
    float f;
    unsigned int ui;
    int i;
  } v;
  struct actor_dict_entry* next;
} actor_dict_entry_t;

typedef struct actor_dict_type {
  unsigned int (*hash)(const void* key);
  int (*key_compare)(const void* key1, const void* key2);
  void* (*key_dup)(const void* key);
  void* (*val_dup)(const void* val);
  void (*key_delete)(void* key);
  void (*val_delete)(void* val);
} actor_dict_type_t;

typedef struct actor_dictht {
  struct actor_dict_entry** table;
  unsigned int size;
  unsigned int sizemask;
  unsigned int used;
} actor_dictht_t;

typedef struct actor_dict {
  actor_dict_type_t* type;
  actor_dictht_t ht[2];
} actor_dict_t;

typedef struct actor_dict_interator {
  actor_dict_t* dict;
  int index;
  actor_dict_entry_t *entry, *next_entry;
} actor_dict_interator_t;

#define actor_dict_hash_key(d, key) ((d)->type->hash(key))
#define actor_dict_compare_key(d, key1, key2) \
  ((d)->type->key_compare ? (d)->type->key_compare(key1, key2) : (key1 == key2))

#define actor_dict_set_key(d, entry, _key_)     \
  do {                                          \
    if (d->type->key_dup) {                     \
      (entry)->key = (d)->type->key_dup(_key_); \
    } else {                                    \
      (entry)->key = _key_;                     \
    }                                           \
  } while (0)

#define actor_dict_set_val(d, entry, _val_)       \
  do {                                            \
    if ((d)->type->val_dup) {                     \
      (entry)->v.val = (d)->type->val_dup(_val_); \
    } else {                                      \
      (entry)->v.val = (_val_);                   \
    }                                             \
  } while (0)

#define actor_dict_free_key(d, entry)      \
  do {                                     \
    if ((d)->type->key_delete) {           \
      (d)->type->key_delete((entry)->key); \
    }                                      \
  } while (0)

#define actor_dict_free_val(d, entry)        \
  do {                                       \
    if ((d)->type->val_delete) {             \
      (d)->type->val_delete((entry)->v.val); \
    }                                        \
  } while (0)

extern actor_dict_t* actor_dict_create(actor_dict_type_t* type);
extern actor_dict_entry_t* actor_dict_add_raw(actor_dict_t* dict,
                                              void* key,
                                              actor_dict_entry_t** existing);
extern int actor_dict_add(actor_dict_t* dict, void* key, void* val);
actor_dict_entry_t* actor_dict_find(actor_dict_t* dict, void* key);
extern void* actor_fetch_value(actor_dict_t* dict, void* key);
extern int actor_dict_delete(actor_dict_t* dict, void* key);
extern void actor_dict_destroy(actor_dict_t* dict);
extern void actor_dict_enmpty(actor_dict_t* dict);
extern actor_dict_interator_t* actor_dict_create_iterator(actor_dict_t* dict);
extern void actor_dict_destroy_iterator(actor_dict_interator_t* iter);
extern actor_dict_entry_t* actor_dict_iterator_next(actor_dict_interator_t* iter);

ACTOR_HASH_TYPE
murmurhash(const char* key, int len, int seed);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif