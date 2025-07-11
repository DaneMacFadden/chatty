#include <list.h>
#include <stdlib.h>
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
  NODE *new_node = malloc(sizeof(NODE));
  if (new_node == NULL || (list->current == NULL && list->size != 0)) {
    return -1;
  }
  new_node->data = item;
  if (list->size == 0) {
    list->current = new_node;
    list->head = new_node;
    list->tail = new_node;
  }
  else if (list->current == list->tail) {
    new_node->previous = list->tail;
    list->tail->next = new_node;
    list->tail = new_node;
    list->current = new_node;
  }
  else if (list->current != list->tail && list->size > 0) {
    new_node->next = list->current->next;
    new_node->previous = list->current;
    list->current->next->previous = new_node;
    list->current->next = new_node;
    list->current = new_node;
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
  NODE *new_node = malloc(sizeof(NODE));
  if (new_node == NULL || (list->current == NULL && list->size != 0)) {
    return -1;
  }

  new_node->data = item;
  if (list->size == 0) {
    list->current = new_node;
    list->head = new_node;
    list->tail = new_node;
  }
  else if (list->current == list->head) {
    new_node->next = list->head;
    list->head->previous = new_node;
    list->head = new_node;
    list->current = new_node;
  }
  else if (list->current != list->head && list->size > 0) {
    new_node->next = list->current;
    new_node->previous = list->current->previous;
    list->current->previous->next = new_node;
    list->current->previous = new_node;
    list->current = new_node;    
  }
  else {
    return -1;
  }

  list->size++;
  return 0;
}

int ListPrepend(LIST *list, void *item) {
  NODE *new_node = malloc(sizeof(NODE));
  if (new_node == NULL) {
    return -1;
  }
  new_node->data = item;
  if (list->size == 0) {
    list->current = new_node;
    list->head = new_node;
    list->tail = new_node;
  }
  new_node->next = list->head;
  list->head->previous = new_node;
  list->head = new_node;
  list->size++;
  return 0;

}

int ListAppend(LIST *list, void *item) {
  NODE *add = malloc(sizeof(NODE));
  if (add == NULL) {
    return -1;
  }
  add->data = item;
  if (list->size == 0) {
    list->current = add;
    list->head = add;
    list->tail = add;
  }
  else {
    add->previous = list->tail;
    list->tail->next = add;
    list->tail = add;
  }
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

