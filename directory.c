

/**
Exemplo de rotina usada para listar todos os nomes de arquivos existentes na área de diretório do T2FS
Utiliza as funções t2fs_first e t2fs_next
*/
void    Directory(void)
{
    int i;
    int count;
    t2fs_find dir;
    t2fs_record rec;

    if (t2fs_first (&dir))
    {
        printf ("Erro ao abrir o diretorio.\n");
        return;
    }

    char *name = malloc(sizeof(rec.name)+1);
    if (name==NULL)
    {
        printf ("Erro ao ler o diretorio.\n");
        return;
    }

    count=0;
    while ( (i=t2fs_next(&dir, &rec)) == 0 )
    {
        printf ("%s\n", (char *)rec.name);
        count++;
    }

    free(name);

    if (i!=1)
    {
        printf ("Erro ao ler o diretorio.\n");
        return;
    }
    if (count==0)
        printf ("Diretorio vazio.\n");
    else
        printf ("\nEncontrados %d arquivos.\n\n", count);
}
