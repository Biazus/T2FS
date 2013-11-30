/*
* typedefs.h: Arquivo com as definições de tipo.
*
* NÃO MODIFIQUE ESTE ARQUIVO.
*/
#ifndef __typedefs__
#define __typedefs__ 1

#include "t2fs.h"


typedef struct st_TCB TCB;
struct st_TCB
{
	t2fs_file handler;
	t2fs_record record;
};

#endif
