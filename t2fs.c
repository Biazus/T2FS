#include <stdio.h>
#include <string.h>
#include "t2fs.h"

int geometryLoaded = 0;
char ctrlSize = 1;
int diskSize = 256;
short int blockSize = 256;
short int freeBlockSize = 1;
short int rootSize = 16;
short int fileEntry = 64;

void InvalidateRootDirectory()
{
    int i=0, dirty = 0, iBloco = 0, lengthBlocoControl = 0;
    char block[blockSize];

    lengthBlocoControl = ctrlSize + freeBlockSize;

    for(iBloco = 0; iBloco < rootSize; iBloco++)
    {
        read_block(lengthBlocoControl + iBloco, block);
        for(i=0; i<blockSize;i+=64)
        {
            if(block[i] < 161 || block[i] > 250)
            {
                block[i] = 0;
                dirty = 1;
            }
        }
        if(dirty)
            write(lengthBlocoControl + iBloco, block);
    }
}

void GetDiskInformation()
{
    int i;
    if(!geometryLoaded)
    {
        geometryLoaded = 1;
        char block[256];
        read_block(0,block);
        if(block[0] == 'T' && block[1] == '2' && block[2] == 'F' && block[3] == 'S')
        {
            ctrlSize = block[5];
            diskSize = *((int *)(block + 6));
            blockSize = *((short int *)(block + 10));
            freeBlockSize = *((short int *)(block + 12));
            rootSize = *((short int *)(block + 14));
            fileEntry = *((short int *)(block + 16));

            InvalidateRootDirectory();
        }
        else
        {
            printf("\n Not a T2FS disk!!!! \n");
            exit(1);
        }
    }
}

int InsertFileRecord(t2fs_record* record)
{
    int i=0, j=0, dirty = 0, iBloco = 0, lenBlkCtrl = 0;
    char block[blockSize];
    lenBlkCtrl = ctrlSize + freeBlockSize;

    for(iBloco = 0; iBloco < rootSize; iBloco++)
    {
        read_block(lenBlkCtrl + iBloco, block);
        for (i = 0; i < blockSize; i+=64)
        {
            if((unsigned char)block[i] < 161 || (unsigned char)block[i] > 250)
            {
                printf("primeiroCaracter = %d, achou no i=%d e iBloco=%d\n", (unsigned char)block[i], i, iBloco);
                memcpy(block+i, record, sizeof(*record));
                block[i] += 128;
                printf("\nPrimeiro caracter = %d\n", (unsigned char)block[i]);
                dirty = 1;
                break;
            }
        }
        if(dirty)
        {
            printf("Salvo no bloco %d\n", lenBlkCtrl + iBloco);
            write_block(lenBlkCtrl + iBloco, block);
            break;
        }
    }

    return 0;
}

t2fs_file t2fs_create (char *nome)
{
    GetDiskInformation();

    char diretorioBlock[blockSize];
    t2fs_record newFile;
    memcpy(newFile.name, nome, sizeof(nome));
    newFile.name[39] = 0;
    newFile.blocksFileSize = 0;
    newFile.bytesFileSize = 0;
    newFile.dataPtr[0] = 0;
    newFile.dataPtr[1] = 0;
    newFile.singleIndPtr = 0;
    newFile.doubleIndPtr = 0;

    InsertFileRecord(&newFile);

    t2fs_file f = 0;
    return f;
}