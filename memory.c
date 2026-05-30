/*
 * memory.c
 * Administrador de memoria: modelo de bloques fijos.
 * Cada bloque es de BLOCK_SIZE bytes; hay MAX_MEMORY_BLOCKS bloques totales.
 */

#include <stdio.h>
#include <string.h>
#include "memory.h"

static MemoryBlock mem_table[MAX_MEMORY_BLOCKS];

/* ---------------------------------------------------------- */
void memory_init(void)
{
    int i;
    for (i = 0; i < MAX_MEMORY_BLOCKS; i++) {
        mem_table[i].block_id = i;
        mem_table[i].pid      = -1;
        mem_table[i].in_use   = 0;
        memset(mem_table[i].data, 0, BLOCK_SIZE);
    }
    printf("[MEM] Memoria inicializada: %d bloques x %d bytes.\n",
           MAX_MEMORY_BLOCKS, BLOCK_SIZE);
}

/* ---------------------------------------------------------- */
/* Asigna el primer bloque libre al proceso pid.
   Retorna el block_id asignado o -1 si no hay espacio. */
int memory_allocate(int pid)
{
    int i;
    for (i = 0; i < MAX_MEMORY_BLOCKS; i++) {
        if (!mem_table[i].in_use) {
            mem_table[i].in_use = 1;
            mem_table[i].pid    = pid;
            printf("[MEM] Bloque %d asignado al proceso %d.\n", i, pid);
            return i;
        }
    }
    printf("[MEM] ERROR: No hay bloques libres para PID %d.\n", pid);
    return -1;
}

/* ---------------------------------------------------------- */
/* Libera todos los bloques asignados al proceso pid.
   Retorna el numero de bloques liberados. */
int memory_free(int pid)
{
    int i, count = 0;
    for (i = 0; i < MAX_MEMORY_BLOCKS; i++) {
        if (mem_table[i].in_use && mem_table[i].pid == pid) {
            mem_table[i].in_use = 0;
            mem_table[i].pid    = -1;
            memset(mem_table[i].data, 0, BLOCK_SIZE);
            count++;
        }
    }
    printf("[MEM] %d bloque(s) liberado(s) del proceso %d.\n", count, pid);
    return count;
}

/* ---------------------------------------------------------- */
void memory_show(void)
{
    int i;
    printf("\n--- Estado de Memoria (%d bloques) ---\n", MAX_MEMORY_BLOCKS);
    printf("%-8s %-8s %-8s\n", "Bloque", "PID", "Estado");
    printf("-----------------------------\n");
    for (i = 0; i < MAX_MEMORY_BLOCKS; i++) {
        printf("%-8d %-8d %-8s\n",
               mem_table[i].block_id,
               mem_table[i].pid,
               mem_table[i].in_use ? "OCUPADO" : "LIBRE");
    }
    printf("-----------------------------\n\n");
}

/* ---------------------------------------------------------- */
int memory_blocks_used(void)
{
    int i, count = 0;
    for (i = 0; i < MAX_MEMORY_BLOCKS; i++)
        if (mem_table[i].in_use) count++;
    return count;
}
