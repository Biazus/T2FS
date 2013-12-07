T2FS

[PRONTO (Guilherme)] t2fs_create_file()
    [PRONTO (Guilherme)] - Lista para armazenar os descritores de arquivos abertos

[PRONTO (Naiche)] não criar dois arquivos com o mesmo nome e gravar bloco como usado no bitmap.
    [Removido (Guilherme)] - Segundo especificação, se o arquivo existir, o mesmo deve ser substituído.
                
[PRONTO (Naiche)] allocateBlock()

[PRONTO (Naiche)] t2fs_write() para ponteiros diretos
[PRONTO (Naiche)] t2fs_write() para indireção simples

[PRONTO (Naiche)] t2fs_delete() até indireção simples

[PRONTO (Naiche)] t2fs_read() até indireção simples

[FAZENDO (Naiche)] Testes
                  
[PRONTO (Guilherme)] t2fs_open
[PRONTO (Naiche)] t2fs_open armazena endereço do arquivo no descritor

[PRONTO (Guilherme)] t2fs_close

[PRONTO (Naiche)] Correções para arquivos grandes
[PRONTO (Naiche)] escrita reestruturada 
[PRONTO (Naiche)] correção dos WARNINGS. 

[DEVE SER FEITO] Nao deixar abrir mais de uma vez o mesmo arquivo antes de fechá-lo.
[DEVE SER FEITO] Corrigir o t2fs_create que nao está salvando os dados no vetor descritores_abertos corretamente
[DEVE SER FEITO] findStruct
[DEVE SER FEITO] t2fs_first
[DEVE SER FEITO] t2fs_next
