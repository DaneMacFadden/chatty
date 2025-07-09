#include <list.h>
#include <stdlib.h>
#include <stdio.h>

LIST *ListCreate(void) {
  LIST *new_list;  
  
  new_list = malloc(sizeof(LIST));
  if (!new_list) {
    return NULL;
  }
  
  new_list->size = 0;
  new_list->current = 0;
  new_list->head = 0;
  new_list->tail = 0;

  return new_list;
}

int ListAdd(LIST *list, void *item) {
  /* 3 cases
   * 1. Empty list
   * 2. Insert when cursor is at the tail (& list not empty)
   * 3. Insert when cursor is anywhere but the tail (& list not empty)
   */
  NODE *new = malloc(sizeof(NODE));
  if (new == NULL || (list->current == NULL && list->size != 0)) {
    return -1;
  }
  new->data = item;
  if (list->size == 0) {
    list->current = new;
    list->head = new;
    list->tail = new;
  }
  else if (list->current == list->tail) {
    new->previous = list->tail;
    list->tail->next = new;
    list->tail = new;
    list->current = new;
  }
  else if (list->current != list->tail && list->size > 0) {
    new->next = list->current->next;
    new->previous = list->current;
    list->current->next->previous = new;
    list->current->next = new;
    list->current = new;
  }
  else {
    return -1;
  }

  list->size++;
  return 0;
}

int ListInsert(LIST *list, void *item) {
  /* 3 cases
   * 1. Empty list
   * 2. Insert when cursor is at the head (& list not empty)
   * 3. Insert when cursor is anywhere but the head (& list not empty)
   */
  NODE *new = malloc(sizeof(NODE));
  if (new == NULL || (list->current == NULL && list->size != 0)) {
    return -1;
  }

  new->data = item;
  if (list->size == 0) {
    list->current = new;
    list->head = new;
    list->tail = new;
  }
  else if (list->current == list->head) {
    new->next = list->head;
    list->head->previous = new;
    list->head = new;
    list->current = new;
  }
  else if (list->current != list->head && list->size > 0) {
    new->next = list->current;
    new->previous = list->current->previous;
    list->current->previous->next = new;
    list->current->previous = new;
    list->current = new;    
  }
  else {
    return -1;
  }

  list->size++;
  return 0;
}

int ListPrepend(LIST *list, void *item) {
  NODE *new = malloc(sizeof(NODE));
  if (new == NULL) {
    return -1;
  }
  new->data = item;
  new->next = list->head;
  list->head->previous = new;
  list->head = new;
  list->size++;
  return 0;

}

int ListAppend(LIST *list, void *item) {
  NODE *new = malloc(sizeof(NODE));
  if (new == NULL) {
    return -1;
  }
  new->data = item;
  new->previous = list->tail;
  list->tail->next = new;
  list->tail = new;
  list->size++;
  return 0;

}

void ListConcat(LIST *listone, LIST *listtwo) {
  listone->tail->next = listtwo->head;
  listone->tail = listtwo->tail;
  listone->size += listtwo->size;
  free(listtwo);
  listtwo = NULL;
}

