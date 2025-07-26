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
 * Create an empty list
 * Return: Pointer to new empty list
 */
LIST *ListCreate(void);

/*
 * Get the number of items in the list
 * Return: Integer containing number of items
 */
int ListCount(LIST *list);

/* 
 * Get the first item in the list and make it the current item
 * list: Pointer to a list
 * Return: Pointer to head's data
 */
void *ListFirst(LIST *list);

/*
 * Get the last item in the list and make it the current item
 * list: Pointer to a list
 * Return: Pointer to tail's data
 */
void *ListLast(LIST *list);

/* Get the item in the list after the cursor and make the cursor point to it
 * list: Pointer to a list
 * Return: Pointer to next item's data
 */
void *ListNext(LIST *list);

/*
 * Get the item before the cursor and set the cursor to it.
 * list: Pointer to a list
 * Return: Pointer to previous item's data
 */
void *ListPrev(LIST *list);

/* Get current item in list
 * list: Pointer to list
 * Return: Pointer to current item's data
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
 * Insert an item right before the current item, switch cursor to new item.
 * If current is the start of the list, add to the start.
 * list: Pointer to list
 * item: Pointer to item to add
 * Return: 0 if successful, -1 otherwise
 */
int ListInsert(LIST *list, void *item);

/*
 * Add an item to the end of the list. Keep cursor where it is.
 * list: Pointer to list
 * item: Pointer to item to add
 * Return: 0 if successful, -1 otherwise
 */
int ListAppend(LIST *list, void *item);

/* 
 * Add item to the front of the list
 * list: Pointer to list
 * item: Pointer to item to add
 * Return: 0 if successful, -1 otherwise
 */
int ListPrepend(LIST *list, void *item);

/* 
 * Delete current item from list, free current item, and make next item current
 * list: Pointer to list
 * Return: 0 on success, -1 on failure 
 */
int ListRemove(LIST *list);

/*
 * Adds list2 to the end of list1
 * Current pointer set to current pointer of list 1. List2 ceases to exist
 * list1: Pointer to list1
 * list2: Pointer to list2
 */
void ListConcat(LIST *list1, LIST *list2);

/*
 * Deletes list
 * Ensure you set your pointer to NULL after calling so you don't get
 * dangling pointers!
 * list: Pointer to list
 */
void ListFree(LIST *list);

/* 
 * Returns last item and takes it out of the list
 * list: Pointer to list
 * Returns Pointer to last item that was just deleted from list
 */
void *ListTrim(LIST *list);

/*
 * Search list starting at current item. 
 * Finishes at end of list or when a match is found
 * list: Pointer to list
 * comparator: Routine that returns 0 
 * if the item and comparisonArg don't match,
 * or 1 if they do. If a match is found the current pointer 
 * is left at the matched item
 * If no match is found,
 * current pointer is left at the end
 * Return: Pointer to matching item if found, otherwise NULL pointer
 */
void *ListSearch(LIST *list, int (*comparator)(void *, void *), 
    void *comparisonArg);

#endif


