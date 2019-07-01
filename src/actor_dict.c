#include "actor_common.h"

#define DICT_INITIAL_SIZE 32

static int dict_expand_if_need(actor_dict_t* dict);

static int dict_key_index(actor_dict_t* dict,
                          const void* key,
                          ACTOR_HASH_TYPE hash,
                          actor_dict_entry_t** existing);

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

actor_dict_entry_t* actor_dict_add_raw(actor_dict_t* dict,
                                       void* key,
                                       actor_dict_entry_t** existing) {
  int index =
      dict_key_index(dict, key, actor_dict_hash_key(dict, key), existing);
  if (index == -1) {
    return NULL;
  }
  actor_dict_entry_t* entry = ACTOR_MALLOC(sizeof(actor_dict_entry_t));
  if (entry == NULL) {
    ACTOR_PRINT("actor dict add malloc null\r\n");
    return NULL;
  }
  memset(entry, 0, sizeof(actor_dict_entry_t));
  entry->next = dict->ht[0].table[index];
  dict->ht[0].table[index] = entry;
  dict->ht[0].used++;
  actor_dict_set_key(dict, entry, key);
  return entry;
}

int actor_dict_expand(actor_dict_t* dict, unsigned int size) {
  actor_dictht_t n;
  int rehash_index = 0;
  memset(&n, 0, sizeof(actor_dictht_t));
  n.size = size;
  n.sizemask = size - 1;
  n.table = ACTOR_MALLOC(size * sizeof(actor_dict_entry_t));
  n.used = 0;
  if (n.table == NULL) {
    ACTOR_PRINT("dict expand malloc error\n");
    return -1;
  }
  memset(n.table, 0, size * sizeof(actor_dict_entry_t));
  while (dict->ht[0].used) {
    actor_dict_entry_t *de = NULL, *next_de = NULL;
    ACTOR_ASSERT(dict->ht[0].size > rehash_index);
    while (dict->ht[0].table[rehash_index] == NULL) {
      rehash_index++;
    }
    de = dict->ht[0].table[rehash_index];
    while (de) {
      ACTOR_HASH_TYPE h;
      next_de = de->next;
      h = actor_dict_hash_key(dict, de->key) & n.sizemask;
      de->next = NULL;
      if (n.table[h] == NULL) {
        n.table[h] = de;
      } else {
        de->next = n.table[h];
        n.table[h] = de;
      }
      dict->ht[0].used--;
      n.used++;
      de = next_de;
    }
    dict->ht[0].table[rehash_index] = NULL;
    rehash_index++;
  }
  if (dict->ht[0].table) {
    ACTOR_FREE(dict->ht[0].table);
  }
  dict->ht[0] = n;
  return 0;
}

int actor_dict_add(actor_dict_t* dict, void* key, void* val) {
  actor_dict_entry_t* entry = actor_dict_add_raw(dict, key, NULL);
  if (!entry) {
    return -1;
  }
  actor_dict_set_val(dict, entry, val);
  return 0;
}

actor_dict_entry_t* actor_dict_find(actor_dict_t* dict, void* key) {
  actor_dict_entry_t* entry = NULL;
  dict_key_index(dict, key, actor_dict_hash_key(dict, key), &entry);
  return entry;
}

void actor_dict_enmpty(actor_dict_t* dict) {
  int index = 0;
  while (dict->ht[0].used) {
    if (dict->ht[0].table[index]) {
      actor_dict_entry_t* de = dict->ht[0].table[index];
      while (de) {
        actor_dict_entry_t* next = de->next;
        dict->ht[0].used--;
        actor_dict_free_key(dict, de);
        actor_dict_free_val(dict, de);
        ACTOR_FREE(de);
        de = next;
      }
    }
    dict->ht[0].table[index] = NULL;
    index++;
  }
  return;
}

void* actor_fetch_value(actor_dict_t* dict, void* key) {
  actor_dict_entry_t* entry = NULL;
  dict_key_index(dict, key, actor_dict_hash_key(dict, key), &entry);
  if (entry) {
    return entry->v.val;
  }
  return NULL;
}

int actor_dict_delete(actor_dict_t* dict, void* key) {
  actor_dict_entry_t *de = NULL, *pre_de = NULL;
  int index = actor_dict_hash_key(dict, key) & dict->ht[0].sizemask;
  de = dict->ht[0].table[index];
  while (de) {
    if (actor_dict_compare_key(dict, key, de->key)) {
      if (pre_de) {
        pre_de->next = de->next;
      } else {
        dict->ht[0].table[index] = de->next;
      }
      dict->ht[0].used--;
      actor_dict_free_key(dict, de);
      actor_dict_free_val(dict, de);
      ACTOR_FREE(de);
      break;
    }
    pre_de = de;
    de = de->next;
  }

  return 0;
}

actor_dict_interator_t* actor_dict_create_iterator(actor_dict_t* dict) {
  actor_dict_interator_t* res = ACTOR_MALLOC(sizeof(actor_dict_interator_t));
  if (res == NULL)
    return res;
  memset(res, 0, sizeof(actor_dict_interator_t));
  res->dict = dict;
  return res;
}

void actor_dict_destroy_iterator(actor_dict_interator_t* iter) {
  ACTOR_FREE(iter);
  return;
}

actor_dict_entry_t* actor_dict_iterator_next(actor_dict_interator_t* iter) {
  while (1) {
    if (iter->index >= iter->dict->ht[0].size) {
      break;
    }
    if (iter->entry == NULL) {
      iter->entry = iter->dict->ht[0].table[iter->index++];
    } else {
      iter->entry = iter->next_entry;
    }
    if (iter->entry) {
      iter->next_entry = iter->entry->next;
      return iter->entry;
    }
  }
  return NULL;
}

/**
 * return index if slot null
 * return -1 if key already exist
 */
static int dict_key_index(actor_dict_t* dict,
                          const void* key,
                          ACTOR_HASH_TYPE hash,
                          actor_dict_entry_t** existing) {
  int ret = 0, index = -1;
  actor_dict_entry_t* de = NULL;
  if (existing)
    *existing = NULL;
  ret = dict_expand_if_need(dict);
  if (ret < 0)
    return ret;
  index = hash & dict->ht[0].sizemask;
  de = dict->ht[0].table[index];
  while (de) {
    if (actor_dict_compare_key(dict, key, de->key)) {
      if (existing) {
        *existing = de;
      }
      return -1;
    }
    de = de->next;
  }
  return index;
}

static int dict_expand_if_need(actor_dict_t* dict) {
  int ret = 0;
  if (dict->ht[0].size == 0) {
    ret = actor_dict_expand(dict, DICT_INITIAL_SIZE);
  } else if (dict->ht[0].used * 100 / dict->ht[0].size > 70) {
    ACTOR_PRINT("increase dict size %d\r\n", dict->ht[0].used * 2);
    ret = actor_dict_expand(dict, dict->ht[0].used * 2);
  } else if (dict->ht[0].used * 100 / dict->ht[0].size < 30 &&
             dict->ht[0].size > DICT_INITIAL_SIZE) {
    ACTOR_PRINT("reduce dict size %d used %d\r\n", dict->ht[0].size,
                dict->ht[0].used);
    ret = actor_dict_expand(dict, dict->ht[0].size / 2);
  }
  return ret;
}

ACTOR_HASH_TYPE
murmurhash(const char* key, int len, int seed) {
  ACTOR_HASH_TYPE c1 = 0xcc9e2d51;
  ACTOR_HASH_TYPE c2 = 0x1b873593;
  ACTOR_HASH_TYPE r1 = 15;
  ACTOR_HASH_TYPE r2 = 13;
  ACTOR_HASH_TYPE m = 5;
  ACTOR_HASH_TYPE n = 0xe6546b64;
  ACTOR_HASH_TYPE h = 0;
  ACTOR_HASH_TYPE k = 0;
  unsigned char* d = (unsigned char*)key;  // 32 bit extract from `key'
  const ACTOR_HASH_TYPE* chunks = NULL;
  const unsigned char* tail = NULL;  // tail - last 8 bytes
  int i = 0;
  int l = len / 4;  // chunk length

  h = seed;

  chunks = (const ACTOR_HASH_TYPE*)(d + l * 4);  // body
  tail = (const unsigned char*)(d + l * 4);      // last 8 byte chunk of `key'

  // for each 4 byte chunk of `key'
  for (i = -l; i != 0; ++i) {
    // next 4 byte chunk of `key'
    k = chunks[i];

    // encode next 4 byte chunk of `key'
    k *= c1;
    k = (k << r1) | (k >> (32 - r1));
    k *= c2;

    // append to hash
    h ^= k;
    h = (h << r2) | (h >> (32 - r2));
    h = h * m + n;
  }

  k = 0;

  // remainder
  switch (len & 3) {  // `len % 4'
    case 3:
      k ^= (tail[2] << 16);
    case 2:
      k ^= (tail[1] << 8);

    case 1:
      k ^= tail[0];
      k *= c1;
      k = (k << r1) | (k >> (32 - r1));
      k *= c2;
      h ^= k;
  }

  h ^= len;

  h ^= (h >> 16);
  h *= 0x85ebca6b;
  h ^= (h >> 13);
  h *= 0xc2b2ae35;
  h ^= (h >> 16);

  return h;
}
