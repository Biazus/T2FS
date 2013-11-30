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

struct st_Descritor{
    t2fs_record record;
    int currentPos;
};
typedef struct  st_Descritor Descritor;

Descritor* descritores_abertos[20];
char count_descritores = 0;


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


int fileExists(char *nome, int *posicao)
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
				*posicao = i;
				return (lenBlkCtrl + iBloco);
            }
        }
    }
	//printf("\nNão existe");
    return 0;
}


int DeleteFileContent(int bloco, int posicao)  //apaga o conteúdo de um arquivo
{
	if (bloco < ctrlSize || bloco > (ctrlSize + rootSize - 1))
	{
		printf ("Endereço de arquivo fora do diretório raiz\n");
		return 0;
	}

	//implementar atualizacao do bitmap

	printf("\nDeletando posição %d do Bloco %d\n", posicao, bloco);

	char block[blockSize];
	int iBloco, posByte, i;

    read_block(bloco, block);        //lê o bloco

	for(i = posicao+40; i<posicao+48; i++)
	{
		block[i] = 0;   //zera o tamanho em blocos (40) e em bytes (44)
	}

	write_block(bloco, block);       //escreve no disco

	return 1;
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

    for(iBloco = 0; iBloco < rootSize; iBloco++)  //varre o diretório raiz
    {
        read_block(lenBlkCtrl + iBloco, block);   //lê o bloco
        for (i = 0; i < blockSize; i+=64)		//varre o bloco lido
        {
            if((unsigned char)block[i] < 161 || (unsigned char)block[i] > 250)  //se o registro não for válido, grava o novo registro
            {
                memcpy(block+i, record, sizeof(*record));  //grava o primeiro registro
                block[i] += 128;		//soma 128 no primeiro caracter
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

    if(count_descritores >= 20)
    {
        printf("***********ERRO: Voce ja possui 20 arquivos abertos!\n");
        return -1;
    }

    char *name;
    name = ExtendName(nome);

    Descritor* t = (Descritor*)malloc(sizeof(Descritor));
    memcpy(t->record.name, name, 40);//sizeof(nome));
    t->record.name[39] = 0;
    t->record.blocksFileSize = 0;
    t->record.bytesFileSize = 0;
    t->record.dataPtr[0] = 0;
    t->record.dataPtr[1] = 0;
    t->record.singleIndPtr = 0;
    t->record.doubleIndPtr = 0;

    InsertFileRecord(&t->record);

    descritores_abertos[count_descritores] = t;
    count_descritores++;

    return count_descritores-1;
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
    GetDiskInformation();

    if(count_descritores >= 20)
    {
        printf("\n******ERRO: Voce ja possui 20 arquivos abertos. \n");
        return -1;
    }

    int i=0, found = 0, iBloco = 0, lenBlkCtrl = 0;
    char block[blockSize];
    lenBlkCtrl = ctrlSize + freeBlockSize;    //offset para posição do root

    nome = ExtendName(nome);
    *nome = *nome | 128; //Liga o bit 7 para fazer a pesquisa

    for(iBloco = 0; iBloco < rootSize; iBloco++)  //varre o diretório raiz
    {
        read_block(lenBlkCtrl + iBloco, block);   //lê o bloco
        for (i = 0; i < blockSize; i+=64)       //varre o bloco lido
        {
            printf("i = %d arquivo: %s e Nome pesquisa: %s\n", i, block + i, nome);
            if(strcmp(block + i, nome) == 0)  //compara pelo nome
            {
                printf("encontrado no bloco %d\n", lenBlkCtrl + iBloco);
                Descritor* t = (Descritor*)malloc(sizeof(Descritor));
                t->currentPos = 0;
                memcpy(block+i, &(t->record), sizeof(t->record));  //grava o primeiro registro
                descritores_abertos[count_descritores] = t;
                count_descritores++;
                found = 1;
                return count_descritores - 1;
            }
        }
    }
    printf("\n******ERRO: Não foi possível encontrar o arquivo. \n");
    return - 1;
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

void sair(void)
{
    int i = 0;
    for (i = 0; i < count_descritores; ++i)
    {
        free(descritores_abertos[i]);
    }
}
