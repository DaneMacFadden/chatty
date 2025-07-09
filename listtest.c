#include <list.h>
#include <stdio.h>
#include <stdlib.h>

int ListPrint(LIST *list) {
  if (ListFirst(list) != NULL) {
    printf("%d, ", *(int*)list->current->data);
  }
 
  while (ListNext(list) != NULL) {
    printf("%d, ", *(int*)list->current->data);
  }
  printf("\n");
  return 0;
}

int main(void) {
  LIST *listone = ListCreate();
  LIST *listtwo = ListCreate();
  NODE *cursor, *temp;
  int errors = 0;
  int one = 1;
  int two = 2;
  int three = 3;
  int four = 4;
  int five = 5;
  int six = 6;
  int seven = 7;

  /* Adders tests */
  if (listone == NULL) {
    errors++;
    printf("ListCreate failed on list 1\n");
  }
  if (listtwo == NULL) {
    errors++;
    printf("ListCreate failed on list 2\n");
  }
  
  /* Test each of the three cases for ListAdd */
  ListAdd(listone, &one);
  if (ListCount(listone) != 1) {
    printf("Error in ListAdd: ListCount expected 1 got %d\n", 
            ListCount(listone));
    errors++;
  }
  if (listone->head->data != &one) {
    printf("Error in ListAdd: head data unexpected\n");
    errors++;
  }
  if (listone->tail->data != &one) {
    printf("Error in ListAdd: tail data unexpected\n");
    errors++;
  }
  if (listone->current->data != &one) {
    printf("Error in ListAdd: current data unexpected\n");
    errors++;
  }
 
  /* ListAdd 2 */
  ListAdd(listone, &two);
  if (ListCount(listone) != 2) {
    printf("Error in ListAdd 2: ListCount expected 2 got %d\n", 
            ListCount(listone));
    errors++;
  }
  if (listone->tail->data != &two) {
    printf("Error in ListAdd 2: tail data unexpected\n");
    errors++;
  }
  if (listone->current->data != &two) {
    printf("Error in ListAdd 2: current data unexpected\n");
    errors++;
  }

  /* ListAdd 3 */
  ListFirst(listone);
  if (listone->current != listone->head) {
    printf("Error in ListAdd 3: ListFirst didn't move cursor\n");
    errors++;
  }
  ListAdd(listone, &three);
  if (ListCount(listone) != 3) {
    printf("Error in ListAdd 3: ListCount expected 3 got %d\n", 
            ListCount(listone));
    errors++;
  }
  if (listone->head->data != &one) {
    printf("Error in ListAdd 3: head data unexpected\n");
    errors++;
  }
  if (listone->tail->data != &two) {
    printf("Error in ListAdd 3: tail data unexpected\n");
    errors++;
  }
  if (listone->current->data != &three) {
    printf("Error in ListAdd 3: current data unexpected\n");
    errors++;
  }

  ListLast(listone);
  if (listone->current != listone->tail) {
    printf("Error: ListLast didn't move cursor\n");
    errors++;
  }

  /* List Append */
  ListFirst(listone);
  cursor = listone->current;
  ListAppend(listone, &four);
  if (ListCount(listone) != 4) {
    printf("Error in ListAppend: ListCount expected 4 got %d\n", 
    ListCount(listone));
    errors++;
  }
  if (listone->tail->data != &four) {
    printf("Error in ListAppend: tail data unexpected\n");
    errors++;
  }
  if (listone->current != cursor) {
    printf("Error in ListAppend: Cursor moved");
    errors++;
  }
  
  ListNext(listone);
  if (listone->current->data != &three) {
    printf("Error: ListNext didn't move the cursor\n");
    errors++;
  } 

  /* List Prepend */ 
  cursor = listone->current;
  ListPrepend(listone, &five); 
  if (ListCount(listone) != 5) {
    printf("Error in ListPrepend: ListCount expected 5 got %d\n", 
    ListCount(listone));
    errors++;
  }
  if (listone->head->data != &five) {
    printf("Error in ListPrepend: head data unexpected\n");
    errors++;
  }
  if (listone->current != cursor) {
    printf("Error in ListPrepend: Cursor moved");
    errors++;
  }
   
  temp = listone->current->previous;
  ListPrev(listone);
  if (temp != listone->current) {
    printf("Error: ListPrev did not function properly\n");
    errors++;
  }

  /* List Insert */
  /* Test w/ cursor at head of list */
  ListFirst(listone);
  temp = listone->current;
  ListInsert(listone, &six);

  if (listone->head->data != &six) {
    printf("Error in ListInsert: Did not change head of list\n");
    errors++;
  }

  if (ListCount(listone) != 6) {
    printf("Error: ListInsert did not update count\n");
    errors++;
  }
  
  /* Move cursor and test */
  ListNext(listone);
  ListNext(listone);

  ListInsert(listone, &seven);

  if (ListCount(listone) != 7) {
    printf("Error: ListInsert did not update count\n");
    errors++;
  }
  if (listone->current->data != &seven) {
    printf("Error: ListInsert did not update current\n");
    errors++;
  }
  if (ListCurr(listone) != listone->current->data) {
    printf("Error: ListCurr did not return current data\n");
    errors++;
  }
  
  ListAdd(listtwo, &one);
  ListAdd(listtwo, &two);
  ListAdd(listtwo, &three);
  ListAdd(listtwo, &four);
  ListAdd(listtwo, &five);
  
  ListFirst(listtwo);
  ListNext(listtwo);
  ListNext(listtwo);
  if (ListCurr(listtwo) != &three) {
    printf("Error: List movers\n");
    errors++;
  }
  
  /* Test remove */

  ListRemove(listtwo);
  if (ListCurr(listtwo) != &four) {
    printf("Error: ListRemove did not move the cursor properly\n");
    errors++;
  }
  if (ListCount(listtwo) != 4) {
    printf("Error: ListRemove did not update count\n");
    errors++;
  }

  ListFirst(listtwo);
  ListRemove(listtwo);
  if (listtwo->head->data != &two || listtwo->current->data != &two) {
    printf("Error: ListRemove did not change the head or cursor\n");
    errors++;
  }
  
  ListLast(listtwo);
  ListRemove(listtwo);
  if (listtwo->tail->data != &four || listtwo->current->data != &four) {
    printf("Error: ListRemove did not change the tail or cursor\n");
    errors++;
  }
  
  /* Test concat */
  ListConcat(listone, listtwo);
  listtwo = NULL;
  if (ListCount(listone) != 9) {
    printf("Error: ListConcat did not change list size\n");
    errors++;
  }
  if (listone->tail->previous->data != &two || listone->tail->data != &four) {
    printf("Error: ListConcat did not concatenate properly\n");
    errors++;
  }
 
  /* Tested free in GDB */
  ListTrim(listone);
  if (listone->tail->data != &two) {
    printf("Error: ListTrim did not set tail properly\n");
    errors++;
  }
  if (ListCount(listone) != 8) {
    printf("Error: ListTrim did not change count properly\n");
    errors++;
  }  
  printf("Finished with %d errors\n", errors);
    
  return 0;
}
