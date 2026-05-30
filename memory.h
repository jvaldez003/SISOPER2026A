#ifndef MEMORY_H
#define MEMORY_H

#define MAX_MEMORY_BLOCKS 16
#define BLOCK_SIZE        64   /* bytes per block */

typedef struct {
    int  block_id;
    int  pid;          /* -1 = libre */
    int  in_use;
    char data[BLOCK_SIZE];
} MemoryBlock;

void memory_init(void);
int  memory_allocate(int pid);
int  memory_free(int pid);
void memory_show(void);
int  memory_blocks_used(void);

#endif
