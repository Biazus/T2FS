#include <stdio.h>
#include "t2fs.h"

int geometryLoaded = 0;
int blockSize = 4096;

void GetDiskInformation()
{
    int i;
    if(!geometryLoaded)
    {
        geometryLoaded = 1;
        //TODO: Implementar
    }
}

t2fs_file t2fs_create (char *nome)
{
    GetDiskInformation();
    t2fs_file newFile;
    newFile.name = nome;
    newFile.blocksFileSize = 0;
    newFile.bytesFileSize = 0;
    newFile.dataPtr[0] = 0;
    newFile.dataPtr[0] = 0;
    newFile.singleIndPtr = 0;
    newFile.doubleIndPtr = 0;

    return newFile;
}