/*
    Ejemplo didáctico de Journaling e Inodes
    ANSI C compatible
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_LOG 10
#define BLOCK_SIZE 128

/* ---------------------------
   Estructura tipo inode
   --------------------------- */
typedef struct {
    int inode_number;     /* identificador único */
    int size;             /* tamaño del archivo */
    int block;            /* bloque donde vive */
    int used;             /* en uso */
} Inode;

/* ---------------------------
   Entrada del journal
   --------------------------- */
typedef struct {
    char operation[32];
    int inode;
    int committed;
} JournalEntry;

/* almacenamiento simulado */
char disk_block[BLOCK_SIZE];
Inode inode_table[4];
JournalEntry journal[MAX_LOG];
int log_index = 0;

/* agrega al journal */
void log_start(const char *op, int inode)
{
    strcpy(journal[log_index].operation, op);
    journal[log_index].inode = inode;
    journal[log_index].committed = 0;
    log_index++;
}

/* confirma transacción */
void log_commit()
{
    journal[log_index - 1].committed = 1;
}

/* mostrar journal */
void show_journal()
{
    int i;

    printf("\nJOURNAL:\n");

    for(i = 0; i < log_index; i++)
    {
        printf("Op=%s inode=%d committed=%d\n",
            journal[i].operation,
            journal[i].inode,
            journal[i].committed);
    }
}

/* crear archivo */
void fs_create(int inode_num)
{
    log_start("CREATE", inode_num);

    inode_table[inode_num].inode_number = inode_num;
    inode_table[inode_num].size = 0;
    inode_table[inode_num].block = inode_num;
    inode_table[inode_num].used = 1;

    log_commit();
}

/* escribir archivo */
void fs_write(int inode_num, const char *text)
{
    log_start("WRITE", inode_num);

    strcpy(disk_block, text);
    inode_table[inode_num].size = strlen(text);

    log_commit();
}

/* leer archivo */
void fs_read(int inode_num)
{
    if(inode_table[inode_num].used)
    {
        printf("\nREAD inode %d:\n%s\n",
            inode_num,
            disk_block);
    }
}

/* cerrar archivo */
void fs_close(int inode_num)
{
    log_start("CLOSE", inode_num);
    log_commit();
}

int main()
{
    fs_create(0);
    fs_write(0, "Hola journaling con inodes");
    fs_read(0);
    fs_close(0);

    show_journal();

    return 0;
}