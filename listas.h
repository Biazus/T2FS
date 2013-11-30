/*
* listas.h:
*
* NÃO MODIFIQUE ESTE ARQUIVO.
*/
#include <stdio.h>
#include "typedefs.h"

typedef struct TipoTCBList ItemList;

struct TipoTCBList
{
	TCB* thTCB;
	ItemList* proximo;
};

/*Create a new list
	return: pointer to the first element of the list
*/
ItemList* createList(void);


/*Print tid of all elements from the list on the screen
	ItemList *threadList: pointer to the first element of the list
	return: size of the list if successful
*/
int showList(ItemList *threadList);


/*Inserts an element at the end of the list
	ItemList *threadList: pointer to the first element of the list
	TCB *aThread: TCB of the element to be inserted
	return: size of the list if successful, 0 if not
*/
t2fs_file insertList(ItemList **threadList, TCB *aThread);


/*Gets an element from the list with the desired tid
	ItemList *threadList: pointer to the first element of the list
	int tid: tid of the desired TCB
	return: Pointer to the desired TCB
*/
TCB* getItemById(ItemList *threadList, t2fs_file* tid);


/*Remove an element from the list
	ItemList *threadList = pointer to the first element of the list
	int tid = id of the item to be removed
	return: a pointer to the removed item
*/
TCB* removeFromList(ItemList **threadList, t2fs_file* tid);


/*Remove the first element from the list
	ItemList *threadList = pointer to the first element of the list
	return: a pointer to the removed item
*/
TCB* removeFirstList(ItemList **threadList);


/*Destroy a list
	ItemList *threadList: pointer to the first element of the list
	return: Null if successful
*/
ItemList* destroy(ItemList* threadList);
