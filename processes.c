/*
 * processes.c
 * Planificador / administrador de procesos simplificado.
 * Mantiene una tabla de PCB de tamano fijo (MAX_PROCESSES).
 */

#include <stdio.h>
#include <string.h>
#include "processes.h"

static PCB  proc_table[MAX_PROCESSES];
static int  next_pid = 1;

static const char *state_str(ProcessState s)
{
    switch (s) {
        case PROC_READY:      return "LISTO";
        case PROC_RUNNING:    return "EJECUTANDO";
        case PROC_BLOCKED:    return "BLOQUEADO";
        case PROC_TERMINATED: return "TERMINADO";
        default:              return "DESCONOCIDO";
    }
}

/* ---------------------------------------------------------- */
void processes_init(void)
{
    int i;
    for (i = 0; i < MAX_PROCESSES; i++) {
        proc_table[i].active       = 0;
        proc_table[i].pid          = 0;
        proc_table[i].memory_block = -1;
        proc_table[i].state        = PROC_TERMINATED;
        memset(proc_table[i].name, 0, MAX_NAME_LEN);
    }
    next_pid = 1;
    printf("[PROC] Tabla de procesos inicializada (%d entradas).\n",
           MAX_PROCESSES);
}

/* ---------------------------------------------------------- */
int process_create(const char *name)
{
    int i;
    for (i = 0; i < MAX_PROCESSES; i++) {
        if (!proc_table[i].active) {
            proc_table[i].active       = 1;
            proc_table[i].pid          = next_pid++;
            proc_table[i].state        = PROC_READY;
            proc_table[i].memory_block = -1;
            strncpy(proc_table[i].name, name, MAX_NAME_LEN - 1);
            proc_table[i].name[MAX_NAME_LEN - 1] = '\0';
            printf("[PROC] Proceso '%s' creado con PID %d.\n",
                   proc_table[i].name, proc_table[i].pid);
            return proc_table[i].pid;
        }
    }
    printf("[PROC] ERROR: Tabla de procesos llena.\n");
    return -1;
}

/* ---------------------------------------------------------- */
int process_terminate(int pid)
{
    int i;
    for (i = 0; i < MAX_PROCESSES; i++) {
        if (proc_table[i].active && proc_table[i].pid == pid) {
            proc_table[i].active = 0;
            proc_table[i].state  = PROC_TERMINATED;
            printf("[PROC] Proceso PID %d ('%s') terminado.\n",
                   pid, proc_table[i].name);
            return 0;
        }
    }
    printf("[PROC] ERROR: PID %d no encontrado.\n", pid);
    return -1;
}

/* ---------------------------------------------------------- */
int process_set_state(int pid, ProcessState state)
{
    int i;
    for (i = 0; i < MAX_PROCESSES; i++) {
        if (proc_table[i].active && proc_table[i].pid == pid) {
            proc_table[i].state = state;
            return 0;
        }
    }
    return -1;
}

/* ---------------------------------------------------------- */
PCB *process_get(int pid)
{
    int i;
    for (i = 0; i < MAX_PROCESSES; i++)
        if (proc_table[i].active && proc_table[i].pid == pid)
            return &proc_table[i];
    return NULL;
}

/* ---------------------------------------------------------- */
void processes_show(void)
{
    int i;
    printf("\n--- Tabla de Procesos ---\n");
    printf("%-6s %-16s %-12s %-8s\n", "PID", "Nombre", "Estado", "Mem-Blk");
    printf("------------------------------------------\n");
    for (i = 0; i < MAX_PROCESSES; i++) {
        if (proc_table[i].active) {
            printf("%-6d %-16s %-12s %-8d\n",
                   proc_table[i].pid,
                   proc_table[i].name,
                   state_str(proc_table[i].state),
                   proc_table[i].memory_block);
        }
    }
    printf("------------------------------------------\n\n");
}

/* ---------------------------------------------------------- */
int processes_active_count(void)
{
    int i, count = 0;
    for (i = 0; i < MAX_PROCESSES; i++)
        if (proc_table[i].active) count++;
    return count;
}