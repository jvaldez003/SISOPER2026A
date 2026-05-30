#ifndef PROCESSES_H
#define PROCESSES_H

#define MAX_PROCESSES  8
#define MAX_NAME_LEN   32

typedef enum {
    PROC_READY,
    PROC_RUNNING,
    PROC_BLOCKED,
    PROC_TERMINATED
} ProcessState;

typedef struct {
    int          pid;
    char         name[MAX_NAME_LEN];
    ProcessState state;
    int          memory_block; /* block_id asignado, -1 si ninguno */
    int          active;       /* 1 = existe en tabla */
} PCB;                         /* Process Control Block */

void processes_init(void);
int  process_create(const char *name);
int  process_terminate(int pid);
int  process_set_state(int pid, ProcessState state);
PCB *process_get(int pid);
void processes_show(void);
int  processes_active_count(void);

#endif
