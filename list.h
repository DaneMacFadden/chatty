/* 
 Dane MacFadden
 James Hom
*/

#ifndef __LIST_H__
#define __LIST_H__


typedef struct node {
	void *data;
	struct node *next;
	struct node *previous;
} NODE;

typedef struct list {
	
	int size;
	NODE *current;
	NODE *head;
	NODE *tail;
	
} LIST;

/* 
 Create empty list
 Returns: Pointer to new empty list if successful, NULL pointer otherwise
 */
LIST *ListCreate();


/* Returns the number of items contained by the list */
int ListCount(LIST *list);


/* 
 Returns pointer to first item in the list and makes it the current item
 list: Pointer to a list
 Returns: Pointer to first item in the list
*/
void *ListFirst(LIST *list);


/*
 Return pointer to last item in list
 list: Pointer to a list
 Returns: Pointer to last item in list
*/
void *ListLast(LIST *list);


/*
 Return next item in the list
 list: Pointer to a list
 Returns: Pointer to next item in list
*/

void *ListNext(LIST *list);


/*
 Return previous item in list
 list: Pointer to a list
 Returns: Pointer to last item
*/
void *ListPrev(LIST *list);


/* Get current item in list
 list: Pointer to list
 Returns: Pointer to last item
*/
void *ListCurr(LIST *list);


/*
 Add item to list after current item, 
 switch current item to the item we just added.
 If current item is at the end, item is added at the end.
 list: List pointer
 item: Pointer to an item to add to the list
 Returns: 0 if successful, -1 otherwise
*/
int ListAdd(LIST *list, void *item);


/*
 Insert an item right before the current item.
 If current is the start of the list, add to the start.
 list: Pointer to list
 item: Pointer to item to add
 Returns: 0 if successful, -1 otherwise
 */
int ListInsert(LIST *list, void *item);


/*
 Add an item to the end of the list
 list: Pointer to list
 item: Pointer to item to add
 Returns: 0 if successful, -1 otherwise
*/
int ListAppend(LIST *list, void *item);


/* 
 Add item to the front of the list
 list: Pointer to list
 item: Pointer to item to add
 Returns: 0 if successful, -1 otherwise
*/
int ListPrepend(LIST *list, void *item);

/* 
 Delete current item from list and make next item current
 list: Pointer to list
 Returns: Deleted item
*/
void *ListRemove(LIST *list);


/*
 Adds list2 to the end of list1
 Current pointer set to current pointer of list1
 List2 ceases to exist
 list1: Pointer to list1
 list2: Pointer to list2
*/
void ListConcat(LIST *list1, LIST *list2);

/*
 Deletes list
 list: Pointer to list
 itemFree: routine that frees an item
*/
void ListFree(LIST *list, void (*itemFree)(void *));


/* 
 Returns last item and takes it out of the list
 list: Pointer to list
 Returns: Pointer to last item that was just deleted from list
*/
void *ListTrim(LIST *list);


/*
 Search list starting at current item. 
 Finishes at end of list or when a match is found
 list: Pointer to list
 comparator: Routine that returns 0 
 if the item and comparisonArg don't match,
 or 1 if they do. If a match is found the current pointer 
 is left at the matched item
 If no match is found,
 current pointer is left at the end
 Returns: Pointer to matching item if found, otherwise NULL pointer
*/
void *ListSearch(LIST *list, int (*comparator)(void *, void *), void *comparisonArg);

#endif


