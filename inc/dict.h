#ifndef __ACTOR_DICT_H__
#define __ACTOR_DICT_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
#include "actor_common.h"

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

extern actor_dict_t* actor_dict_create(actor_dict_type_t* type);
extern int actor_dict_add(actor_dict_t* dict, void* key, void* val);
actor_dict_entry_t* actor_dict_find(actor_dict_t* dict, void* key);
extern void* actor_fetch_value(actor_dict_t* dict, void* key);
extern int actor_dict_delete(actor_dict_t* dict, void* key);
extern void actor_dict_destroy(actor_dict_t* dict);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif