#ifndef __ACTOR_LIST_H__
#define __ACTOR_LIST_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief 链表节点
 */
typedef struct alist_node {
  struct alist_node* prev;  ///<前一个
  struct alist_node* next;  ///<后一个
} alist_node_t;


inline void alist_init(alist_node_t* l) {
  l->next = l->prev = l;
}

inline void alist_insert_after(alist_node_t* l, alist_node_t* n) {
  l->next->prev = n;
  n->next = l->next;

  l->next = n;
  n->prev = l;
}

inline void alist_insert_before(alist_node_t* l, alist_node_t* n) {
  l->prev->next = n;
  n->prev = l->prev;

  l->prev = n;
  n->next = l;
}

inline void alist_remove(alist_node_t* n) {
  n->next->prev = n->prev;
  n->prev->next = n->next;

  n->next = n->prev = n;
}

inline int alist_isempty(const alist_node_t* l) {
  return l->next == l;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
