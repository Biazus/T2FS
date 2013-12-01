#include <stdio.h>
#include "t2fs.h"

int main()
{
    //printf("Handle do arquivo1 criado = %d\n",
	//t2fs_create("arq1");
	int hndl;
	hndl = t2fs_open("arq1");
             printf("Handle do arq1 aberto = %d\n", hndl);

	t2fs_write(hndl, "qwertyuiopasdfghjkl;sdfghjklwertyuioxcvbnm,sertyuiopwertyui1234567", 65);

    t2fs_close(0);

    sair();
    return 0;
}
