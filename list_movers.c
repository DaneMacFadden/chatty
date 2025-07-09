#include <list.h>
#include <stddef.h>

int ListCount(LIST *list) {  
  return (int) list->size;
}

void *ListFirst(LIST *list) {
  list->current = list->head;
  return list->current->data;
}

void *ListLast(LIST *list) {
  list->current = list->tail;
  return list->current->data;
}

void *ListNext(LIST *list) {
  if (list->current->next != NULL) {
    list->current = list->current->next;
    return list->current->data;
  }
  return NULL;
}

void *ListPrev(LIST *list) {
  if (list->current->next != NULL) {
    list->current = list->current->previous;
    return list->current->data;
  }
  return NULL;
}

void *ListCurr(LIST *list) {
  if (list->current != NULL) {
    return list->current->data;
  }
  return NULL;
}

