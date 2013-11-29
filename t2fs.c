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

int setBitBitmap(int posicao, short int ocupado)   //seta ou reseta um bit do bitmap
{
    char block[blockSize];
	int iBloco, posBit, posByte, iAux;

	iBloco = ctrlSize + posicao/(8*blockSize);		//posição do bloco que contém o bit desejado
	printf("\nBit no bloco: %d ", iBloco); 
		
    read_block(iBloco, block);        //lê o bloco
	
	iAux = (posicao - ((iBloco-1) * blockSize));
		
	posByte = iAux / 8;

	posBit = iAux % 8;	    
	
	char auxByte;
	auxByte = block[posByte];
	printf("\nBloco antes: %x - 1 deslocado: %d - posBit: %d ", block[posByte], (1 << posBit), posBit);
	if (ocupado)
		block[posByte] = auxByte | (1 << posBit);
	else
		block[posByte] = auxByte & (254 << posBit);
			
	printf("\nBloco depois: %x ", block[posByte]);   
	//block[posByte] = ; //setar o bit
	
	printf(" - Bit %d do byte: %d\n", posBit, posByte);
    
	write_block(iBloco, block);       //escreve no disco
    
    return 0;
}


int fileExists(char *nome)
{
    int i=0, iBloco = 0, lenBlkCtrl = 0;
	int j;
    char block[blockSize];
    lenBlkCtrl = ctrlSize + freeBlockSize;    //offset para posição do root

    for(iBloco = 0; iBloco < rootSize; iBloco++)  //varre o diretório raiz
    {
        read_block(lenBlkCtrl + iBloco, block);   //lê o bloco
        for (i = 0; i < blockSize; i+=64)		//varre o bloco lido
        {
	    	char fileName[40];
	    	fileName[0] = block[i] - 128;
	    	for(j=1; j<40; j++)
				fileName[j] = block[i+j];

			//printf("\nComparando %s com %s\n", nome, fileName);
        	if(!strcmp(nome, fileName))  //se o registro tem o nome procurado, retorna sua posição
       		{
				//printf("\nJá existe na posição %d ", (iBloco) * blockSize + i);              	
				return (lenBlkCtrl + iBloco) * blockSize + i;
            }
        }
    }
	//printf("\nNão existe");
    return 0;
}


int DeleteFileContent(int posicao)  //apaga o conteúdo de um arquivo
{
	
}


void InvalidateRootDirectory()
{
    int i=0, dirty = 0, iBloco = 0, lengthBlocoControl = 0;
    char block[blockSize];

    lengthBlocoControl = ctrlSize + freeBlockSize;    //offset para posição do root

    for(iBloco = 0; iBloco < rootSize; iBloco++)	//varre a área de diretório
    {
        read_block(lengthBlocoControl + iBloco, block);   //lê o bloco
        for(i=0; i<blockSize;i+=64)			//varre o bloco
        {
            if(block[i] < 161 || block[i] > 250)       //se for arquivo válido
            {
                //printf("Valor do primeiro caracter: %d\n", block[i]);
		//printf("Posição %d invalidada.\n", i);
            	block[i] = 0;				//invalida o arquivo
                dirty = 1;
            }
        }
        if(dirty)
	{
	    //printf("Bloco %d gravado.\n", lengthBlocoControl + iBloco);
            write_block(lengthBlocoControl + iBloco, block);  //escreve no disco
	}
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

            //InvalidateRootDirectory();
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
    int i=0, dirty = 0, iBloco = 0, lenBlkCtrl = 0;
    char block[blockSize];
    lenBlkCtrl = ctrlSize + freeBlockSize;    //offset para posição do root
	printf("\nNome: = %s\n", record->name);

    for(iBloco = 0; iBloco < rootSize; iBloco++)  //varre o diretório raiz
    {
        read_block(lenBlkCtrl + iBloco, block);   //lê o bloco
        for (i = 0; i < blockSize; i+=64)		//varre o bloco lido
        {
            if((unsigned char)block[i] < 161 || (unsigned char)block[i] > 250)  //se o registro não for válido, grava o novo registro
            {
                printf("\nPrimeiroCaracter = %d, achou no i=%d e iBloco=%d\n", (unsigned char)block[i], i, iBloco);
                memcpy(block+i, record, sizeof(*record));  //grava o primeiro registro
                block[i] += 128;		//soma 128 no primeiro caracter
                printf("\nPrimeiro caracter novo = %d\n", (unsigned char)block[i]);
                dirty = 1;
                break;
            }
        }
        if(dirty)
        {
            printf("Salvo no bloco %d\n", lenBlkCtrl + iBloco);
            write_block(lenBlkCtrl + iBloco, block);       //escreve no disco
			setBitBitmap(iBloco + lenBlkCtrl, 1);            
			break;
        }
    }

    return 0;
}

char* ExtendName(char *nome)
{
    char* extName = (char *)malloc(40);
    int i = 0;
	    
    while (nome[i] != 0)
    {
		extName[i] = nome[i]; 
		i++;	
	}
	while (i<40)
    {
		extName[i] = 0; 
		i++;	
	}
	return (extName);
}

t2fs_file t2fs_create (char *nome)
{
    GetDiskInformation();
    
	char *name;
    name = ExtendName(nome);

    char diretorioBlock[blockSize];
    t2fs_record newFile;
    memcpy(newFile.name, name, 40);//sizeof(nome));
    newFile.name[39] = 0;
    newFile.blocksFileSize = 0;
    newFile.bytesFileSize = 0;
    newFile.dataPtr[0] = 0;
    newFile.dataPtr[1] = 0;
    newFile.singleIndPtr = 0;
    newFile.doubleIndPtr = 0;

    if (fileExists(name))
    {
		printf("Arquivo já existe.");
		t2fs_file f = 0;
    	return f; 
    }
	
    InsertFileRecord(&newFile);

    t2fs_file f = 0;
    return f;
}


int t2fs_delete (char *nome)
{	
	GetDiskInformation();	
	//t2fs_first(findStruct);
	
		//colocar 0 no primeiro bit do nome do arquivo
	//percorrer inodes para atualizar bitmap
}

t2fs_file t2fs_open (char *nome)
{
	t2fs_record arquivo;
	strcpy(arquivo.name, nome);
}

int t2fs_close (t2fs_file handle)
{

}

//read

//write

//posiciona o contador na posiçao do offset dentro do arquivo
int t2fs_seek (t2fs_file handle, unsigned int offset)
{
		
}

//localiza o primeiro arquivo válido do diretório
int t2fs_first (t2fs_find *find_struct)
{
	
}

//obtém o próximo registro válido do diretório
int t2fs_next (t2fs_find *findStruct, t2fs_record *dirFile)
{

}
