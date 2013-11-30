#include <stdio.h>
#include "t2fs.h"

int main()
{
    //printf("Handler do arquivo1 criado = %d\n", t2fs_create("arquivo 1"));
    printf("Handler do arq1 aberto = %d\n", t2fs_open("arq1"));

    t2fs_close(0);

    sair();
    return 0;
}