#include "listas.h"


ItemList* createList(void)
{
	return NULL;
}


int showList(ItemList *threadList)
{
    int length = 0;
    ItemList *ptaux = threadList; //ponteiro auxiliar para percorrer a lista
    //printf("ptaux: %d\n", ptaux);

    if (ptaux == NULL)
    {
        return length;
    }
    else
    {
        length++;
        while (ptaux->proximo != NULL)      	//procura o fim da lista
        {
             ptaux = ptaux->proximo;
             length++;
        }
    }
    return length;
}


t2fs_file insertList(ItemList **threadList, TCB *aThread)
{
	ItemList *novo; //novo elemento
	t2fs_file length = 0;

    novo = (ItemList*) malloc(sizeof(ItemList));
	novo->thTCB = aThread;
	novo->proximo = NULL;

	if (*threadList == NULL)
	{
		*threadList = novo;
	}
    else
	{
        ItemList *ptaux;
        ptaux = *threadList; //ponteiro auxiliar para percorrer a lista
             length++;
		while (ptaux->proximo != NULL)      	//procura o fim da lista
		{
			ptaux = ptaux->proximo;
			length++;
 		}
		ptaux->proximo = novo;
	}
             novo->thTCB->handler = length;
	return length;
}


TCB* getItemById(ItemList *threadList, t2fs_file* tid)
{
	ItemList *ptaux;
    	ptaux = threadList; //ponteiro auxiliar para percorrer a lista

	while (ptaux != NULL)      	//procura o elemento na lista
	{
		if(ptaux->thTCB->handler == *(tid))
			return ptaux->thTCB;
		ptaux = ptaux->proximo;
	}
	return NULL;
}


TCB* removeFromList(ItemList **threadList, t2fs_file* tid)
{
	ItemList *ant = NULL; //ponteiro auxiliar para a posição anterior
	ItemList *ptaux;
    ptaux = *threadList; //ponteiro auxiliar para percorrer a lista


    while (ptaux != NULL && ptaux->thTCB->handler != *(tid))      	//procura o elemento na lista
    {
		ant = ptaux;
		ptaux = ptaux->proximo;
 	}

    if (ptaux == NULL)  //se nao achou
		return NULL;

    if (ant == NULL)
	{
		*threadList = ptaux->proximo;  //Remove o primeiro elemento
		return ptaux->thTCB;
	}
	else
	{
		ant->proximo = ptaux->proximo;  //Remove o elemento desejado
    	return ptaux->thTCB;
	}
}


TCB* removeFirstList(ItemList **threadList)
{
	ItemList *ptaux; //ponteiro auxiliar para salvar o endereco do primeiro
    ptaux = *threadList;

    if (*threadList == NULL)  //se nao achou
	{
		return NULL;
	}
    	else
	{
		*threadList = (*threadList)->proximo;  //Remove o primeiro elemento
		return ptaux->thTCB;
	}
}


ItemList* destroy(ItemList *threadList)
{
    ItemList *ptaux; //ponteiro auxiliar para percorrer a lista

    while(threadList != NULL)
    {
         ptaux = threadList;
         threadList = threadList->proximo;
         free(ptaux);
    }
    free(threadList);
    return NULL;

}
