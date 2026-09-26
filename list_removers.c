#include <list.h>
#include <stdlib.h>

int ListRemove(LIST *list) {
  NODE *unused;
  if (list->current == NULL) {
    return -1;
  }
  if (list->size == 1) {
    unused = list->current;
    list->current = NULL;
    list->head = NULL;
    list->tail = NULL;
  }
  else if (list->current == list->head) {
    unused = list->head;
    list->head = list->head->next;
    list->current = list->head;
    list->current->previous = NULL;
  }
  else if (list->current == list->tail) {
    unused = list->tail;
    list->tail= list->tail->previous;
    list->current = list->tail;
    list->current->next = NULL;
  }
  else {
    unused = list->current;
    list->current->previous->next = list->current->next;
    list->current->next->previous = list->current->previous;
    list->current = list->current->next;
  }
  
  free(unused->data);
  free(unused);
  list->size--;
  return 0;

}

void ListFree(LIST *list) {
  ListFirst(list);
  while (list->head != NULL) {
    ListRemove(list);
  }
  free(list);

}

void *ListTrim(LIST *list) {
  NODE *tempnode = list->tail->previous;
  void *nodedata = list->tail->data;
  list->tail->previous = NULL;
  free(list->tail);
  list->tail = tempnode;
  list->size--;
  return nodedata;
}

void *ListSearch(LIST *list, int (*comparator)(void *, void*), 
    void *comparisonArg) {
  ListFirst(list);
  while (list->current != NULL) {
    if ((*comparator)(list->current->data, comparisonArg)) {
      break;
    }
    else {
      ListNext(list);
    }
  }

  if (list->current == NULL) {
    list->current = list->tail;
    return NULL;
  }
  
  return list->current;
}
