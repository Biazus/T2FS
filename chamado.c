#include <stdio.h>
#include "t2fs.h"

int main()
{
    int i;
    char sperBlock[4096];
    for (i = 0; i < 4096; ++i)
    {
        sperBlock[i] = 0;
    }
    read_block(0,&sperBlock);
    for (i = 0; i < 4096; ++i)
    {
        printf("%02x ", sperBlock[i]);
    }
    printf("\nHUE\n");
    return 0;
}