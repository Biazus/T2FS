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
	int bloco;
	int posNoBloco;
    t2fs_record record;
    t2fs_file handler;
    int currentPos;
};
typedef struct  st_Descritor Descritor;

Descritor* descritores_abertos[20];
char count_descritores = 0;
t2fs_file next_handler = 0;


Descritor* getDescritorByHandle(t2fs_file handle)
{
    int i;
    for(i=0; i<count_descritores; i++)
    {
        if(descritores_abertos[i]->handler == handle){
            return descritores_abertos[i];
        }
    }
    return NULL;
}


int setBitBitmap(int posicao, short int ocupado)   //seta ou reseta um bit do bitmap
{
    char block[blockSize];
	int iBloco, posBit, posByte, iAux;

	iBloco = ctrlSize + posicao/(8*blockSize);		//posição do bloco que contém o bit desejado
	printf("\nBit no bloco: %d ", iBloco);

    read_block(iBloco, block);        //lê o bloco

	iAux = (posicao - ((iBloco-1) * blockSize));

	posByte = iAux / 8;

	posBit = 7-(iAux % 8);

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


int allocateBlock()    //Aloca um bloco da área de dados e índices retornando seu endereço
{
	char block[blockSize], auxByte;
	int i, posByte, posBit, bitmapBlock, inicioDados;

	bitmapBlock = ctrlSize;
	inicioDados = ctrlSize + freeBlockSize + rootSize;

	read_block(bitmapBlock, block);        //lê o bloco
	for (i = inicioDados; i < diskSize; i++) //varre o bitmap
	{
		if (i > (bitmapBlock-ctrlSize+1)*blockSize*8)  //caso exista mais de um bloco de bitmap
		{
			bitmapBlock++;
			read_block(bitmapBlock, block);
		}
		posByte = (i - (bitmapBlock-ctrlSize)) / 8;
		posBit = 7 - (i % 8);

		auxByte = block[posByte] & (1 << posBit);

		printf("\nByte lido: %x - 1 desloc.: %x - posBit: %d - i: %d - posByte: %d", block[posByte], (1 << posBit), posBit, i, posByte);

		if (auxByte == 0)
		{
			setBitBitmap(i, 1);
			printf("\nBloco %d alocado.\n", i);
			return i;
		}
	}
	return -1;
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
        for (i = 0; i < blockSize; i+=64)		 //varre o bloco lido
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

    t->handler = next_handler;

    next_handler++;

    descritores_abertos[count_descritores] = t;
    count_descritores++;

    return t->handler;
}


int t2fs_delete (char *nome)
{
	GetDiskInformation();
	char block[blockSize];
	
	int hndl, qtdBlocos, i=0;
	hndl = t2fs_open(nome);
	qtdBlocos = descritores_abertos[hndl]->record.blocksFileSize;

	while(qtdBlocos > 0 && i<2)
	{
		setBitBitmap(descritores_abertos[hndl]->record.dataPtr[i], 0);
		i++;		
		qtdBlocos--;
	}
	
	i = 0;
	if(qtdBlocos > 0)
	{
		setBitBitmap(descritores_abertos[hndl]->record.singleIndPtr, 0);
		read_block(descritores_abertos[hndl]->record.singleIndPtr, block);
		while(qtdBlocos > 0 && i<blockSize)
		{
			setBitBitmap(block[i], 0);
			i++;		
			qtdBlocos--;
		}
	}

	i = 0;
	if(qtdBlocos > 0)
	{
		setBitBitmap(descritores_abertos[hndl]->record.doubleIndPtr, 0);
		read_block(descritores_abertos[hndl]->record.doubleIndPtr, block);
		/*while(qtdBlocos > 0 && i<blockSize)     //liberar blocos de indirecao dupla
		{
			setBitBitmap(block[i], 0);
			i++;		
			qtdBlocos--;
		}*/
	}
	
	read_block(descritores_abertos[hndl]->bloco, block);	
	block[descritores_abertos[hndl]->posNoBloco] = 0;	//colocar 0 no primeiro bit do nome do arquivo
	write_block(descritores_abertos[hndl]->bloco, block);	
	
	t2fs_close(hndl);
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
            //printf("i = %d arquivo: %s e Nome pesquisa: %s\n", i, block + i, nome);
            if(strcmp(block + i, nome) == 0)  //compara pelo nome
            {
                printf("encontrado no bloco %d\n", lenBlkCtrl + iBloco);
                Descritor* t = (Descritor*)malloc(sizeof(Descritor));
				t->bloco = lenBlkCtrl + iBloco;
				t->posNoBloco = i;
				t->currentPos = 0;
                t->handler = next_handler;

                next_handler++;

                memcpy(&(t->record), block+i, sizeof(t->record));  //grava o primeiro registro
                descritores_abertos[count_descritores] = t;
                count_descritores++;
                found = 1;
                return t->handler;;
            }
        }
    }
    printf("\n******ERRO: Não foi possível encontrar o arquivo. \n");
    return - 1;
}

int t2fs_close (t2fs_file handle)
{
    int i =0, j=0;
    for (i = 0; i < 20; ++i)
    {
        if(descritores_abertos[i]->handler == handle){
            free(descritores_abertos[i]);
            for(j=i+1;  j<20; j++)
            {
                descritores_abertos[j - 1] = descritores_abertos[j];
            }
            count_descritores--;
            break;
        }
    }
}

//read

int t2fs_write(t2fs_file handle, char *buffer, int size)	//escreve size bytes do buffer no arquivo identificado por handle
{
	int tamAtual, blockAddress, i=0, j, sizeLeft, spaceLeft, addrPoint;
	char block[blockSize];

	int tamOriginal = descritores_abertos[handle]->record.bytesFileSize;

	if (size > 2*blockSize + blockSize*blockSize + blockSize*blockSize*blockSize)
	{
		printf("\nTamanho máximo de arquivo excedido.");
		return -1;
	}

	sizeLeft = size;
	spaceLeft = (descritores_abertos[handle]->record.blocksFileSize * blockSize) - descritores_abertos[handle]->record.bytesFileSize;
	tamAtual = descritores_abertos[handle]->record.bytesFileSize;

	while (sizeLeft >0)
	{
		if (spaceLeft == 0)		//alocar um bloco de dados
		{
			addrPoint = 0;
			blockAddress = allocateBlock();
			if (blockAddress < 1)
			{
				printf("Erro ao alocar bloco.");
				return -1;
			}
			descritores_abertos[handle]->record.blocksFileSize++;
			spaceLeft = blockSize;
			if (tamAtual==0)
				descritores_abertos[handle]->record.dataPtr[0]  = blockAddress;
			else if (tamAtual < 2*blockSize)
				descritores_abertos[handle]->record.dataPtr[1]  = blockAddress;
			else if (tamAtual == 2*blockSize)// && tamAtual < (2+blockSize)*blockSize) //cria o bloco de índice da indireçao simples
			{
				int blockAddressInd;
				blockAddressInd = allocateBlock();   //aloca bloco de índice (indireção simples)
				if (blockAddressInd < 1)
				{
					printf("Erro ao alocar bloco de índice.");
					return -1;
				}
				descritores_abertos[handle]->record.singleIndPtr = blockAddressInd;
				
				char blockPtr[blockSize];
				blockPtr[0] = blockAddress;
				for (j=1; j<blockSize; j++) blockPtr[j] = 0;
				write_block(blockAddressInd, blockPtr);	//grava bloco de índice
			}
			else if ((tamAtual > 2*blockSize) && (tamAtual < (2+blockSize)*blockSize)) //usa o bloco de índice da indireçao simples
			{
				char blockPtr[blockSize];				
				j = descritores_abertos[handle]->record.blocksFileSize - 3;
				read_block(descritores_abertos[handle]->record.singleIndPtr, blockPtr);
				blockPtr[j] = blockAddress;
				printf("\n%d Ptr:", j);
				for(j=0; j<256;j++) printf(" %d", blockPtr[j]);
				write_block(descritores_abertos[handle]->record.singleIndPtr, blockPtr);	//grava bloco de índice
			}
			else
			{
				//indireçao dupla
			}			
		}
		else		//localiza o último bloco de dados do arquivo
		{
			addrPoint = tamAtual % blockSize;
			if (tamAtual<blockSize)
				blockAddress = descritores_abertos[handle]->record.dataPtr[0];
			else if (tamAtual < 2*blockSize)
				blockAddress = descritores_abertos[handle]->record.dataPtr[1];
			else if (tamAtual > 2*blockSize && tamAtual < (2+blockSize)*blockSize)
			{
				int auxInd;
				auxInd = descritores_abertos[handle]->record.singleIndPtr;   //bloco de índice (indireção simples)
				char blockInd[blockSize];				
				read_block(auxInd, blockInd);
				//auxInd = (tamAtual - 2*blockSize) / blockSize;
				auxInd = (descritores_abertos[handle]->record.blocksFileSize) - 3;
				blockAddress = blockInd[auxInd];
	printf("\nQtd. blocos: %d, auxInd: %d, blockAddress: %d", descritores_abertos[handle]->record.blocksFileSize, auxInd, blockAddress);
			}
			else
			{
				//indireçao dupla
			}
			read_block(blockAddress, block);		//lê o último bloco			
		}
		printf("\n%d - ", blockAddress);
		while (addrPoint<blockSize && sizeLeft>0) 		//preenche o bloco
		{
			block[addrPoint] = buffer[i];
			printf("%c", buffer[i]);
			i++;
			addrPoint++;
			sizeLeft--;
			spaceLeft--;
		}
		write_block(blockAddress, block);  //escreve dados no disco
		tamAtual = tamOriginal + size - sizeLeft;
	}	
	descritores_abertos[handle]->record.bytesFileSize = tamAtual;
	descritores_abertos[handle]->record.blocksFileSize = tamAtual / blockSize;
	if (addrPoint<blockSize) descritores_abertos[handle]->record.blocksFileSize++; //atualiza descritor
		
	read_block(descritores_abertos[handle]->bloco, block);
	//printf("Conteúdo: \n\n Bloco: %d", descritores_abertos[handle]->bloco);	
	memcpy(block+descritores_abertos[handle]->posNoBloco, &(descritores_abertos[handle]->record), 64);
	write_block(descritores_abertos[handle]->bloco, block);		//atualiza record no root
	
	return size;
}


//posiciona o contador na posiçao do offset dentro do arquivo
int t2fs_seek (t2fs_file handle, unsigned int offset)
{
    Descritor* rec = getDescritorByHandle(handle);
    if(rec == NULL)
    {
        printf("\n*******ERRO: Hande invalido!\n");
        return -1;
    }
    if(Descritor->record.bytesFileSize > offset)
    {
        printf("\n*******ERRO: Offset invalido!\n");
        return -1;
    }
    rec->currentPost = offset;
    return 0;
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
    printf("\ncount_descritores = %d\n", count_descritores);
    for (i = 0; i < count_descritores; ++i)
    {
        free(descritores_abertos[i]);
    }
}
