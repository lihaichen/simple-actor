#include "actor_common.h"

actor_dict_t* actor_dict_create(actor_dict_type_t* type) {
  actor_dict_t* dict = NULL;
  if (type == NULL)
    goto breakout;
  dict = ACTOR_MALLOC(sizeof(*dict));
  if (dict == NULL)
    goto breakout;
  dict->type = type;
  for (int i = 0; i < 2; i++) {
    dict->ht[i].size = 0;
    dict->ht[i].table = NULL;
    dict->ht[i].sizemask = 0;
    dict->ht[i].used = 0;
  }
breakout:
  return dict;
}

void actor_dict_destroy(actor_dict_t* dict) {
  for (int i = 0; i < 2; i++) {
    if (dict->ht[i].table) {
      ACTOR_FREE(dict->ht[i].table);
    }
  }
  ACTOR_FREE(dict);
  return;
}
