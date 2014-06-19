#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "t2fs.h"

int geometryLoaded = 0;
char ctrlSize = 1;
int diskSize = 256;
short int blockSize = 256;
short int freeBlockSize = 1;
short int rootSize = 16;
short int fileEntry = 64;
char ident[]="Mateus Cardoso 000000 Miller Biazus 187984";

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

Descritor* getDescritorByName(char* nome)
{
    int i;
	nome = ExtendName(nome);
    for(i=0; i<count_descritores; i++)
    {
		char nomet[40];
		strcpy(nomet, descritores_abertos[i]->record.name);
		nomet[0] -= 128;
		//printf("testando: %s - %s", nomet, nome);
        if(!strcmp(nomet, nome)){
			//printf("Achou %d", i);
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
	//printf("\nBit no bloco: %d ", iBloco);

    	read_sector(iBloco, block);        //lê o bloco

	iAux = (posicao - ((iBloco-1) * blockSize));

	posByte = iAux / 8;

	posBit = 7-(iAux % 8);

	char auxByte;
	auxByte = block[posByte];
	//printf("\nBloco antes: %x - 1 deslocado: %d - posBit: %d ", block[posByte], (1 << posBit), posBit);
	if (ocupado)
		block[posByte] = auxByte | (1 << posBit);
	else
		block[posByte] = auxByte & (254 << posBit);

	//printf("\nBloco depois: %x ", block[posByte]);
	//block[posByte] = ; //setar o bit

	//printf(" - Bit %d do byte: %d\n", posBit, posByte);

	write_sector(iBloco, block);       //escreve no disco

    return 0;
}


int allocateBlock()    //Aloca um bloco da área de dados e índices retornando seu endereço
{
	char block[blockSize], auxByte;
	int i;
	int posByte, posBit, bitmapBlock, inicioDados;

	bitmapBlock = ctrlSize;
	inicioDados = ctrlSize + freeBlockSize + rootSize;

	read_sector(bitmapBlock, block);        //lê o bloco
	for (i = inicioDados; i < diskSize; i++) //varre o bitmap
	{
		if (i > (bitmapBlock-ctrlSize+1)*blockSize*8)  //caso exista mais de um bloco de bitmap
		{
			bitmapBlock++;
			read_sector(bitmapBlock, block);
		}
		posByte = (i - (bitmapBlock-ctrlSize)) / 8;
		posBit = 7 - (i % 8);

		auxByte = block[posByte] & (1 << posBit);

	//printf("\nByte lido: %x - 1 desloc.: %x - posBit: %d - i: %d - posByte: %d", block[posByte], (1 << posBit), posBit, i, posByte);

		if (auxByte == 0)
		{
			setBitBitmap(i, 1);
			//printf("\nBloco %d alocado.\n", i);
			return i;
		}
	}
	return -1;
}


int fileExists(char *nome, int *posicao)	//testa se já existe arquivo com o este nome no root e retorna posicao de seu bloco
{
    int i=0, iBloco = 0, lenBlkCtrl = 0;
	int j;
    char block[blockSize];
    lenBlkCtrl = ctrlSize + freeBlockSize;    //offset para posição do root

    for(iBloco = 0; iBloco < rootSize; iBloco++)  //varre o diretório raiz
    {
        read_sector(lenBlkCtrl + iBloco, block);   //lê o bloco
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
        read_sector(lengthBlocoControl + iBloco, block);   //lê o bloco
        for(i=0; i<blockSize;i+=64)			//varre o bloco
        {
            if(block[i] < (char)161 || block[i] > (char)250)       //se for arquivo válido
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
            write_sector(lengthBlocoControl + iBloco, block);  //escreve no disco
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
        read_sector(0,block);
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

int InsertFileRecord(t2fs_record* record, int *blocoPtr, int *posPtr)
{
    int i=0, dirty = 0, iBloco = 0, lenBlkCtrl = 0;
    char block[blockSize];
    lenBlkCtrl = ctrlSize + freeBlockSize;    //offset para posição do root

    for(iBloco = 0; iBloco < rootSize; iBloco++)  //varre o diretório raiz
    {
        read_sector(lenBlkCtrl + iBloco, block);   //lê o bloco
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
            //printf("Salvo no bloco %d\n", lenBlkCtrl + iBloco);
            write_sector(lenBlkCtrl + iBloco, block);       //escreve no disco
			setBitBitmap(iBloco + lenBlkCtrl, 1);
			*blocoPtr = lenBlkCtrl + iBloco;
			*posPtr = i;
			break;
        }
    }

    return 0;
}


t2fs_file t2fs_create (char *nome)
{
    GetDiskInformation();

    if(count_descritores >= 20)
    {
        printf("***********ERRO: Voce ja possui 20 arquivos abertos!\n");
        return -1;
    }

	int retorno;
	retorno = t2fs_delete(nome);
	if (retorno == 0)
	{
		printf("O arquivo anterior de mesmo nome foi excluído.\n");
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
	t->currentPos = 0;

    InsertFileRecord(&t->record, &t->bloco, &t->posNoBloco);

    t->handler = next_handler;

    next_handler++;

    descritores_abertos[count_descritores] = t;
    count_descritores++;

	//printf("Tamanho em blocos: %d", t->record.blocksFileSize);
	//printf("Tamanho em blocos: %d", descritores_abertos[count_descritores]->record.blocksFileSize);

    return t->handler;
}


int t2fs_delete (char *nome)
{
	GetDiskInformation();
	char block[blockSize];

	unsigned short pos;
	int hndl, qtdBlocos, i=0;

    Descritor* arquivo = getDescritorByName(nome); //se arquivo já estiver aberto, nao abre de novo
	if (arquivo == NULL)
	{
		//printf("Abrindo arquivo.");
		hndl = t2fs_open(nome);
		arquivo = getDescritorByHandle(hndl);
		if (hndl<0) return -1;
	}
	else
	{
		hndl = arquivo->handler;
		//printf("\nHandle do arquivo a excluir: %d", hndl);
	}

	qtdBlocos = arquivo->record.blocksFileSize;

	while(qtdBlocos > 0 && i<2)  //desaloca o dois primeiros blocos
	{
		setBitBitmap(arquivo->record.dataPtr[i], 0);
		i++;
		qtdBlocos--;
	}

	i = 0;
	if(qtdBlocos > 0)	//desaloca os blocos da indireção simples
	{
		setBitBitmap(arquivo->record.singleIndPtr, 0);
		read_sector(arquivo->record.singleIndPtr, block);
		while(qtdBlocos > 0 && i<blockSize)
		{
			pos = (int)(block[i]);
			if (pos>127) pos += 256;
			//printf("\napagando bloco: %d - %d", block[i], pos);
			setBitBitmap(pos, 0);
			i++;
			qtdBlocos--;
		}
	}

	i = 0;
	if(qtdBlocos > 0)	//desaloca os blocos da indireção dupla
	{
		setBitBitmap(arquivo->record.doubleIndPtr, 0); //desaloca o 1o nível
		read_sector(arquivo->record.doubleIndPtr, block);
		int blockSegNivel = 0;
		char blockInd[blockSize];
		while(qtdBlocos > 0 && blockSegNivel<blockSize)     //liberar blocos de indirecao dupla
		{
			read_sector(block[blockSegNivel], blockInd);
			while(qtdBlocos > 0 && i<blockSize)     //liberar blocos de indirecao dupla
			{
				setBitBitmap(blockInd[i], 0);
				i++;
				qtdBlocos--;
			}
			blockSegNivel++;
		}
	}

	read_sector(arquivo->bloco, block);
	block[arquivo->posNoBloco] = 0;	//colocar 0 no primeiro bit do nome do arquivo
	write_sector(arquivo->bloco, block);

	t2fs_close(hndl);
	return 1;
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
        read_sector(lenBlkCtrl + iBloco, block);   //lê o bloco
        for (i = 0; i < blockSize; i+=64)       //varre o bloco lido
        {
            //printf("i = %d arquivo: %s e Nome pesquisa: %s\n", i, block + i, nome);
            if(strcmp(block + i, nome) == 0)  //compara pelo nome
            {
                //printf("encontrado no bloco %d\n", lenBlkCtrl + iBloco);
                Descritor* t = (Descritor*)malloc(sizeof(Descritor));
				t->bloco = lenBlkCtrl + iBloco;
				t->posNoBloco = i;
				t->currentPos = 0;
                t->handler = next_handler;

                next_handler++;

                memcpy(&(t->record), block+i, sizeof(t->record));  //grava o primeiro registro
				t->record.name[0] -=128;
                descritores_abertos[count_descritores] = t;
                count_descritores++;
                found = 1;
                return t->handler;;
            }
        }
    }
    //printf("\n******ERRO: Não foi possível encontrar o arquivo. \n");
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


int alocarBlocoParaArquivo(Descritor* arquivo, int tamAtual)
{
	//printf("ABA:");
	int blockAddress,j;
	blockAddress = allocateBlock();
	if (blockAddress < 1)
	{
		return -1;
	}
	arquivo->record.blocksFileSize++;

	if (tamAtual==0)
		arquivo->record.dataPtr[0]  = blockAddress;
	else if (tamAtual < 2*blockSize)
		arquivo->record.dataPtr[1]  = blockAddress;
	else if (tamAtual == 2*blockSize) //cria o bloco de índice da indireçao simples
	{
		int blockAddressInd;
		blockAddressInd = allocateBlock();   //aloca bloco de índice (indireção simples)
		if (blockAddressInd < 1)
		{
			return -1;
		}
		arquivo->record.singleIndPtr = blockAddressInd;

		char blockPtr[blockSize];
		blockPtr[0] = blockAddress;
		for (j=1; j<blockSize; j++) blockPtr[j] = 0;
		write_sector(blockAddressInd, blockPtr);	//grava bloco de índice
	}
	else if ((tamAtual > 2*blockSize) && (tamAtual < (2+blockSize)*blockSize)) //usa o bloco de índice da indireçao simples
	{
		char blockPtr[blockSize];
		j = arquivo->record.blocksFileSize - 3;
		read_sector(arquivo->record.singleIndPtr, blockPtr);
		blockPtr[j] = blockAddress;
		//printf("\n%d Ptr:", j);
		//for(j=0; j<256;j++) printf(" %d", blockPtr[j]);
		write_sector(arquivo->record.singleIndPtr, blockPtr);	//grava bloco de índice
	}
	else if (tamAtual == (2+blockSize)*blockSize)  //aloca o bloco de índices da indireção dupla
	{
		int blockAddressInd, blockAddressInd2;
		blockAddressInd = allocateBlock();   //aloca bloco de índice (indireção dupla)
		if (blockAddressInd < 1)
		{	return -1;	}
		arquivo->record.doubleIndPtr = blockAddressInd;

		char blockPtr[blockSize];
		blockAddressInd2 = allocateBlock();   //aloca bloco de índice nivel 2 (indireção dupla)
		if (blockAddressInd2 < 1)
		{	return -1;	}
		blockPtr[0] = blockAddressInd2;
		for (j=1; j<blockSize; j++) blockPtr[j] = 0;
		write_sector(blockAddressInd, blockPtr);	//grava bloco de índice nivel 1

		blockPtr[0] = blockAddress;
		for (j=1; j<blockSize; j++) blockPtr[j] = 0;
		write_sector(blockAddressInd2, blockPtr);	//grava bloco de índice nivel 2
	}
	else if (tamAtual > (2+blockSize)*blockSize && (tamAtual % (blockSize*blockSize) == 0)) 
	{		//aloca o bloco de índices de nivel 2 da indireção dupla
		int blockAddressInd2, posInd1;
		blockAddressInd2 = allocateBlock();   //aloca bloco de índice nivel 2 (indireção dupla)
		if (blockAddressInd2 < 1)
		{
			return -1;
		}
		posInd1 = (arquivo->record.blocksFileSize - blockSize - 2)/blockSize;
		char blockPtr[blockSize];
		read_sector(arquivo->record.doubleIndPtr, blockPtr);
		blockPtr[posInd1] = blockAddressInd2;
		write_sector(arquivo->record.doubleIndPtr, blockPtr);	//grava bloco de índice nivel 1

		blockPtr[0] = blockAddress;
		for (j=1; j<blockSize; j++) blockPtr[j] = 0;
		write_sector(blockAddressInd2, blockPtr);	//grava bloco de índice nivel 2
	}
	else        //usa indireçao dupla
    {
        int iAux, pos, pos2;
        pos = (arquivo->record.blocksFileSize - blockSize - 2)/blockSize;
        pos2 = tamAtual % blockSize;
        char blockPtr[blockSize];    //bloco de índice nivel 2

        read_sector(arquivo->record.doubleIndPtr, blockPtr);  //lê bloco de índice nível 1
        iAux = blockPtr[pos];
        read_sector(iAux, blockPtr);     //lê bloco de índice nível 2
        blockPtr[pos2] = blockAddress;
        write_sector(iAux, blockPtr);        //grava bloco de índice nível 2
    }
	//printf("aba: %d", blockAddress);
	return blockAddress;
}

int localizarBlocoCorrente(Descritor* arquivo, int posAtual)
{
	int blockAddress;
	if (posAtual<blockSize)
		blockAddress = arquivo->record.dataPtr[0];
	else if (posAtual < 2*blockSize)
		blockAddress = arquivo->record.dataPtr[1];
	else if (posAtual > 2*blockSize && posAtual < (2+blockSize)*blockSize)	//caso a posicao atual esteja na indirecao simples
	{
		int auxInd;
		auxInd = arquivo->record.singleIndPtr;   //bloco de índice (indireção simples)
		char blockInd[blockSize];
		read_sector(auxInd, blockInd);
		//auxInd = (tamAtual - 2*blockSize) / blockSize;
		auxInd = posAtual/blockSize -2; //(arquivo->record.blocksFileSize) - 3;
		blockAddress = blockInd[auxInd];
		//printf("\nQtd. blocos: %d, auxInd: %d, blockAddress: %d", arquivo->record.blocksFileSize, auxInd, blockAddress);
	}
	else	//indireçao dupla
	{
		int auxInd2, pos, pos2;
		auxInd2 = arquivo->record.doubleIndPtr;   //bloco de índice (indireção dupla)
		char blockInd[blockSize];
		read_sector(auxInd2, blockInd);			//lê o 1o bloco de indices
		pos = posAtual/blockSize - 2 - blockSize;
		pos2 = pos/blockSize;
		auxInd2 = blockInd[pos2];
		read_sector(auxInd2, blockInd);   //lê o segundo bloco de indices
		blockAddress = blockInd[pos];
		if (blockAddress<0) blockAddress += 256; //corrige nrs negativos
	}
	//printf("LBC: %d", blockAddress);
	return blockAddress;
}


int t2fs_write(t2fs_file handle, char *buffer, int size)	//escreve size bytes do buffer no arquivo identificado por handle
{
	int blockAddress, i=0, j, sizeLeft, spaceLeft, addrPoint, posAtual;
	char block[blockSize];

	Descritor* arquivo = getDescritorByHandle(handle);
	posAtual = arquivo->currentPos;     //posicao corrente no arquivo
	//printf("\nPosAtual: %d\n", posAtual);
	int tamOriginal = arquivo->record.bytesFileSize; //tamanho do arquivo antes da gravacao

	int tamFinal = tamOriginal - posAtual + size; //so pra testtar se o arquivo nao vai ficar grande demais
	if (tamFinal > (diskSize-ctrlSize-freeBlockSize-rootSize)*blockSize)
	{
		printf("\nArquivo grande demais para o disco.");
		return -1;
	}

	if (size + tamOriginal > 2*blockSize + blockSize*blockSize + blockSize*blockSize*blockSize)
	{
		printf("\nTamanho máximo de arquivo excedido.");
		return -1;
	}

	sizeLeft = size;    //quantos bytes ainda falta escrever
	spaceLeft = (arquivo->record.blocksFileSize * blockSize) - posAtual;  //espaço restante nos blocos alocados para o arquivo desde a posAtual

	while (sizeLeft >0)
	{
		if (spaceLeft == 0)		//alocar um bloco de dados
		{
			blockAddress = alocarBlocoParaArquivo(arquivo, posAtual);
			if (blockAddress < 1)
			{
				printf("Erro ao alocar bloco.");
				return -1;
			}
			addrPoint = 0;
			spaceLeft = blockSize;
		}
		else		//localiza o bloco de dados da posicao atual
		{
			addrPoint = posAtual % blockSize;
			blockAddress = localizarBlocoCorrente(arquivo, posAtual);
			read_sector(blockAddress, block);		//lê o último bloco
		}

		//printf("\n%d - ", blockAddress);
		while (addrPoint<blockSize && sizeLeft>0) 		//preenche o bloco
		{
			block[addrPoint] = buffer[i];
			//printf("%c", buffer[i]);
			i++;
			posAtual++;
			addrPoint++;
			sizeLeft--;
			spaceLeft--;
		}
		write_sector(blockAddress, block);  //escreve dados no disco
	}
	arquivo->currentPos = posAtual;

	if (posAtual > tamOriginal)
	{
		arquivo->record.bytesFileSize = posAtual;
	}
	arquivo->record.blocksFileSize = arquivo->record.bytesFileSize / blockSize;
	if (addrPoint<blockSize) arquivo->record.blocksFileSize++; //atualiza descritor

	read_sector(arquivo->bloco, block);
	arquivo->record.name[0] += 128;
	//printf("Conteúdo: \n\n Bloco: %d", arquivo->bloco);
	memcpy(block+arquivo->posNoBloco, &(arquivo->record), 64);
	write_sector(arquivo->bloco, block);							//atualiza registro no root

	return size;
}


int t2fs_read(t2fs_file handle, char *buffer, int size)	//lê size bytes do arquivo identificado por handle para o buffer
{
	int posAtual, tamanho, bytesLidos=0, blocoLido=0, posNoBloco, blockAddress;
	char block[blockSize];

	Descritor* arquivo = getDescritorByHandle(handle);
	tamanho = arquivo->record.bytesFileSize;
	posAtual = arquivo->currentPos;
//printf("\n%d\n", posAtual);

	while (size>0)
	{
		if (!blocoLido)  //carrega o bloco atual
		{
			posNoBloco = posAtual % blockSize;
			if (posAtual<blockSize)
				blockAddress = arquivo->record.dataPtr[0];
			else if (posAtual < 2*blockSize)
				blockAddress = arquivo->record.dataPtr[1];
			else if (posAtual > 2*blockSize && posAtual < (2+blockSize)*blockSize)   //indireçao simples
			{
				int auxInd;
				auxInd = arquivo->record.singleIndPtr;   //bloco de índice (indireção simples)
				char blockInd[blockSize];
				read_sector(auxInd, blockInd);
				auxInd = posAtual/blockSize - 2;
				blockAddress = blockInd[auxInd];
				if (blockAddress<0) blockAddress += 256;	//corrige nrs negativos
				//printf(" - %d ", blockAddress);
			}
			else 				//indireçao dupla
			{
				int auxInd2, pos, pos2;
				auxInd2 = arquivo->record.doubleIndPtr;   //bloco de índice (indireção dupla)
				char blockInd[blockSize];
				read_sector(auxInd2, blockInd);			//lê o 1o bloco de indices
				pos = posAtual/blockSize - 2 - blockSize;
				pos2 = pos/blockSize;
				auxInd2 = block[pos2];
				read_sector(auxInd2, blockInd);   //lê o segundo bloco de indices
				blockAddress = blockInd[pos];
				if (blockAddress<0) blockAddress += 256; //corrige nrs negativos
			}
			read_sector(blockAddress, block);		//lê o bloco atual
			blocoLido = 1;
		}

		//if
		buffer[bytesLidos] = block[posNoBloco-1];
		//printf("%d ", posNoBloco);
		posAtual++;
		posNoBloco++;			//Atualiza flags
		if (posNoBloco > blockSize)
			blocoLido = 0;
		bytesLidos++;
		size--;
	}

	arquivo->currentPos = posAtual;
	//printf("\nTamanho do arquivo: %d", arquivo->record.bytesFileSize);
	return bytesLidos;
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
    if(rec->record.bytesFileSize < offset)
    {
        printf("\n*******ERRO: Offset invalido!\n Offset: %d, Tam. arquivo: %d\n", offset, rec->record.bytesFileSize);
        return -1;
    }
    rec->currentPos = offset;
    return 0;
}

//localiza o primeiro arquivo válido do diretório
int t2fs_first (t2fs_find *find_struct)
{
    GetDiskInformation();

     int i=0, dirty=0, iBloco = 0, lengthBlocoControl = 0;
    unsigned char block[blockSize];

    lengthBlocoControl = ctrlSize + freeBlockSize;    //offset para posição do root

    find_struct->currentBlock = -1;
    find_struct->posInBlock = -1;

    for(iBloco = 0; iBloco < rootSize; iBloco++)    //varre a área de diretório
    {
        read_sector(lengthBlocoControl + iBloco, block);   //lê o bloco
        for(i=0; i<blockSize;i+=64)         //varre o bloco
        {
            if(block[i] >= (unsigned char)161 && block[i] <= (unsigned char)250)       //se for arquivo válido
            {
                find_struct->currentBlock = iBloco;
                find_struct->posInBlock = i;
                dirty = 1;
            }
        }
    }
    if(dirty != 1)
    {
        find_struct->currentBlock = rootSize;
        find_struct->posInBlock = blockSize;
    }
    return 0;
}

//obtém o próximo registro válido do diretório
int t2fs_next (t2fs_find *findStruct, t2fs_record *dirFile)
{
    int i=0, dirty=0, iBloco = 0, lengthBlocoControl = 0;
    unsigned char block[blockSize];
    lengthBlocoControl = ctrlSize + freeBlockSize;

    if(findStruct->currentBlock == rootSize)
        return 1;

    for(iBloco = findStruct->currentBlock; iBloco < rootSize; iBloco++)    //varre a área de diretório
    {
        read_sector(lengthBlocoControl + iBloco, block);   //lê o bloco
        for(i=findStruct->posInBlock; i<blockSize;i+=64)         //varre o bloco
        {
            if(block[i] >= (unsigned char)161 && block[i] <= (unsigned char)250)       //se for arquivo válido
            {
                if(!dirty)
                {
                    memcpy(block+i, dirFile, sizeof(*dirFile));  //grava o primeiro registro
                    dirty = 1;
                }
                else
                {
                    findStruct->currentBlock = iBloco;
                    findStruct->posInBlock = i;
                    return 0;
                }
            }
        }
        findStruct->posInBlock = 0;
    }
    if(dirty)
    {
        findStruct->currentBlock = rootSize;
        findStruct->posInBlock = i;
        return 0;
    }
    return 1;
}

char *t2fs_identify(void)
{
	return "Mateus Cardoso 000000 Miller Biazus 187984"; //&ident; 
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
