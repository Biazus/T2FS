#include <stdio.h>
#include "t2fs.h"

int main()
{
    printf("Handler do arquivo1 = %d\n", t2fs_create("arquivo 1"));

    sair();
    return 0;
}