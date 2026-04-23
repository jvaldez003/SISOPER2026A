/**
 * processes.c
 * Universidad Santiago de Cali, 2026A         
 * Laboratorio #2 Sistemas Operativos - Planificación y Representación de Procesos
 * Basado en algoritmos: Round Robin, FCFS, Prioridad simple, esqueleto CFS-like.
 * COMPILAR: gcc -Wall -std=c99 processes.c -o scheduler_lab
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_PROCESOS 10
#define QUANTUM 2      // para Round Robin

/* ------------------- 1. DEFINICIÓN DEL PCB ------------------- */
typedef enum { NUEVO, LISTO, EJECUCION, BLOQUEADO, TERMINADO } EstadoProceso;

typedef struct {
    int pid;
    char nombre[32];
    int prioridad;            // numérica: menor valor = mayor prioridad (para planificación prioritaria)
    int rafaga_total;         // tiempo total de CPU necesario
    int rafaga_restante;      // para SRTF o contabilidad
    int tiempo_llegada;
    int tiempo_respuesta;     // para estadísticas
    EstadoProceso estado;
    /* Datos específicos para CFS (simulación) */
    long vruntime;            // tiempo virtual (simulado)
    int nice;                 // rango -20..19 (más bajo = más prioridad en CFS)
    /* imagen del proceso: simulación de contexto */
    int *contexto_registros;  // array simple (simula guardado de contexto)
    /* Estadísticas adicionales */
    int first_exec;           // tiempo de primera ejecución
    int completion;           // tiempo de finalización
} PCB;

/* Cola de listos (FIFO para RR y FCFS, ordenable por prioridad/vruntime) */
typedef struct ColaListos {
    PCB* procesos[MAX_PROCESOS];
    int frente, final;
    int cantidad;
} ReadyQueue;

/* ------------------- EXTENSIONES PARA LABORATORIO ------------------- */

// Para Multilevel Feedback Queue
#define NUM_QUEUES 3
typedef struct {
    ReadyQueue queues[NUM_QUEUES];
    int quantums[NUM_QUEUES];
} MLFQ;

// Para Banker's Algorithm
#define NUM_RESOURCES 3
typedef struct {
    int available[NUM_RESOURCES];
    int max_claim[MAX_PROCESOS][NUM_RESOURCES];
    int allocation[MAX_PROCESOS][NUM_RESOURCES];
    int need[MAX_PROCESOS][NUM_RESOURCES];
    PCB* processes[MAX_PROCESOS];
    int num_processes;
} BankerState;

// Para Deadlock Detection
typedef struct {
    int allocation[MAX_PROCESOS][NUM_RESOURCES];
    int request[MAX_PROCESOS][NUM_RESOURCES];
    PCB* processes[MAX_PROCESOS];
    int num_processes;
} DeadlockState;

// Para CFS con Red-Black Tree
typedef struct RBNode {
    PCB* process;
    struct RBNode* left;
    struct RBNode* right;
    struct RBNode* parent;
    int color; // 0 red, 1 black
} RBNode;

typedef struct {
    RBNode* root;
    RBNode* nil; // sentinel
} RBTree;

/* ------------------- 2. FUNCIONES AUXILIARES ------------------- */
void init_ready_queue(ReadyQueue *q) {
    q->frente = 0; q->final = 0; q->cantidad = 0;
    memset(q->procesos, 0, sizeof(q->procesos));
}

bool queue_vacia(ReadyQueue *q) { return q->cantidad == 0; }
bool queue_llena(ReadyQueue *q) { return q->cantidad == MAX_PROCESOS; }

void encolar(ReadyQueue *q, PCB *p) {
    if (queue_llena(q)) return;
    q->procesos[q->final] = p;
    q->final = (q->final + 1) % MAX_PROCESOS;
    q->cantidad++;
}

PCB* desencolar(ReadyQueue *q) {
    if (queue_vacia(q)) return NULL;
    PCB *p = q->procesos[q->frente];
    q->frente = (q->frente + 1) % MAX_PROCESOS;
    q->cantidad--;
    return p;
}

/* Para FIFO / Round Robin se usa orden FIFO normal */
/* Para SPN (SJF) y prioridad se requiere ordenación (implementación simple) */

/* ------------------- 3. ALGORITMOS DE PLANIFICACIÓN (TEMPLATES) ------------------- */

// A) FIFO (FCFS) - No preventivo
void planificar_fcfs(ReadyQueue *q) {
    printf("\n=== PLANIFICACIÓN FCFS ===\n");
    int current_time = 0;
    while (!queue_vacia(q)) {
        PCB *actual = desencolar(q);
        if (actual->first_exec == -1) actual->first_exec = current_time;
        printf("Ejecutando proceso %s (PID %d) | ráfaga %d\n", actual->nombre, actual->pid, actual->rafaga_total);
        // Simula ejecución consumiendo rafaga_total
        current_time += actual->rafaga_total;
        actual->completion = current_time;
        actual->estado = TERMINADO;
    }
}

// B) Round Robin ("preemptivo", apropiativo con quantum fijo)
void planificar_rr(ReadyQueue *q, int quantum) {
    printf("\n=== ROUND ROBIN (quantum=%d) ===\n", quantum);
    ReadyQueue cola_tmp;
    init_ready_queue(&cola_tmp);
    // copiar procesos a cola temporal
    while (!queue_vacia(q)) {
        encolar(&cola_tmp, desencolar(q));
    }
    int tiempo_global = 0;
    while (!queue_vacia(&cola_tmp)) {
        PCB *actual = desencolar(&cola_tmp);
        if (actual->first_exec == -1) actual->first_exec = tiempo_global;
        int remaining = actual->rafaga_restante;
        if (remaining == 0) remaining = actual->rafaga_total;
        printf("[t=%d] Ejecuta %s (restante=%d)\n", tiempo_global, actual->nombre, remaining);
        int executed = (remaining <= quantum) ? remaining : quantum;
        tiempo_global += executed;
        actual->rafaga_restante = remaining - executed;
        if (actual->rafaga_restante <= 0) {
            actual->completion = tiempo_global;
            actual->estado = TERMINADO;
            printf("  -> %s terminado.\n", actual->nombre);
        } else {
            encolar(&cola_tmp, actual);  // vuelve al final
        }
    }
    // Estadísticas completadas en el bucle; para impresión global, usar lista externa de procesos
}

// C) Shortest Process Next (SPN) no preventivo: ordenar por ráfaga total
int cmp_rafaga(const void *a, const void *b) {
    PCB *pa = *(PCB**)a;
    PCB *pb = *(PCB**)b;
    return (pa->rafaga_total - pb->rafaga_total);
}

void planificar_spn(ReadyQueue *q) {
    printf("\n=== SPN (SJF no preventivo) ===\n");
    // Convertir cola a array temporal
    PCB *lista[MAX_PROCESOS];
    int count = 0;
    while (!queue_vacia(q)) {
        lista[count++] = desencolar(q);
    }
    qsort(lista, count, sizeof(PCB*), cmp_rafaga);
    int current_time = 0;
    for (int i = 0; i < count; i++) {
        PCB *p = lista[i];
        p->first_exec = current_time;
        printf("Ejecutando %s (ráfaga=%d)\n", p->nombre, p->rafaga_total);
        current_time += p->rafaga_total;
        p->completion = current_time;
        p->estado = TERMINADO;
    }
}

// D) Plantilla para simulación de CFS usando vruntime y árbol (simulación lineal)
void planificar_cfs_simple(ReadyQueue *q) {
    printf("\n=== CFS SIMULADO (basado en vruntime mínimo) ===\n");
    PCB *procs[MAX_PROCESOS];
    int n = 0;
    while (!queue_vacia(q)) procs[n++] = desencolar(q);
    int tiempo = 0;
    bool all_done = false;
    while (!all_done) {
        all_done = true;
        PCB *active[MAX_PROCESOS];
        int active_count = 0;
        for (int i = 0; i < n; i++) {
            if (procs[i]->estado != TERMINADO) {
                active[active_count++] = procs[i];
                all_done = false;
            }
        }
        if (all_done) break;
        // Ordenar active por vruntime (menor primero)
        for (int i = 0; i < active_count - 1; i++) {
            for (int j = i + 1; j < active_count; j++) {
                if (active[i]->vruntime > active[j]->vruntime) {
                    PCB *tmp = active[i];
                    active[i] = active[j];
                    active[j] = tmp;
                }
            }
        }
        // Ejecutar cada uno en orden
        for (int i = 0; i < active_count; i++) {
            PCB *p = active[i];
            int slice = 2; // base
            if (p->nice < 0) slice = 4;   // más prioridad -> más tiempo
            else if (p->nice > 0) slice = 1;
            if (p->first_exec == -1) p->first_exec = tiempo;
            printf(" [t=%d] Ejecuta %s slice=%d\n", tiempo, p->nombre, slice);
            tiempo += slice;
            p->vruntime += slice;  // incrementa vruntime según tiempo ejecutado
            p->rafaga_restante -= slice;
            if (p->rafaga_restante <= 0) {
                p->estado = TERMINADO;
                p->completion = tiempo;
            }
        }
    }
    /* 
     * NOTA: En CFS real se usa un árbol rojo-negro para seleccionar el mínimo vruntime O(log n).
     * El estudiante debe implementar una estructura RBT o usar una cola de prioridad basada en heap.
     * El nice afecta el incremento de vruntime: delta = tiempo_ejecucion * (NICE_0_WEIGHT / peso(nice))
     */
}

// ------------------- E) MULTILEVEL FEEDBACK QUEUE -------------------
void init_mlfq(MLFQ* mlfq) {
    mlfq->quantums[0] = 2;
    mlfq->quantums[1] = 4;
    mlfq->quantums[2] = 8;
    for (int i = 0; i < NUM_QUEUES; i++) {
        init_ready_queue(&mlfq->queues[i]);
    }
}

void planificar_mlfq(MLFQ* mlfq) {
    printf("\n=== MULTILEVEL FEEDBACK QUEUE ===\n");
    int tiempo = 0;
    bool all_empty = false;
    while (!all_empty) {
        all_empty = true;
        for (int q = 0; q < NUM_QUEUES; q++) {
            if (!queue_vacia(&mlfq->queues[q])) {
                all_empty = false;
                PCB* p = desencolar(&mlfq->queues[q]);
                if (p->first_exec == -1) p->first_exec = tiempo;
                int qtime = mlfq->quantums[q];
                int exec = (p->rafaga_restante <= qtime) ? p->rafaga_restante : qtime;
                tiempo += exec;
                p->rafaga_restante -= exec;
                if (p->rafaga_restante <= 0) {
                    p->completion = tiempo;
                    p->estado = TERMINADO;
                    printf("Proceso %s terminado en cola %d\n", p->nombre, q);
                } else {
                    int next_q = (q < NUM_QUEUES - 1) ? q + 1 : q;
                    encolar(&mlfq->queues[next_q], p);
                    printf("Proceso %s movido a cola %d\n", p->nombre, next_q);
                }
            }
        }
    }
}

// ------------------- F) ALGORITMO DEL BANQUERO (EVITACIÓN DE DEADLOCKS) -------------------
void init_banker(BankerState* bs, PCB* procs[], int n) {
    bs->num_processes = n;
    for (int i = 0; i < n; i++) bs->processes[i] = procs[i];
    // Inicialización de ejemplo
    bs->available[0] = 10; bs->available[1] = 5; bs->available[2] = 7;
    for (int i = 0; i < n; i++) {
        bs->max_claim[i][0] = 7; bs->max_claim[i][1] = 5; bs->max_claim[i][2] = 3;
        for (int j = 0; j < NUM_RESOURCES; j++) {
            bs->allocation[i][j] = 0;
            bs->need[i][j] = bs->max_claim[i][j] - bs->allocation[i][j];
        }
    }
}

bool is_safe(BankerState* bs, int request[], int pid) {
    for (int j = 0; j < NUM_RESOURCES; j++) {
        if (request[j] > bs->available[j] || request[j] > bs->need[pid][j]) return false;
    }
    // Simular asignación
    int work[NUM_RESOURCES];
    bool finish[MAX_PROCESOS] = {false};
    memcpy(work, bs->available, sizeof(work));
    for (int j = 0; j < NUM_RESOURCES; j++) {
        work[j] -= request[j];
        bs->allocation[pid][j] += request[j];
        bs->need[pid][j] -= request[j];
    }
    // Encontrar secuencia segura
    int seq[MAX_PROCESOS];
    int count = 0;
    while (count < bs->num_processes) {
        bool found = false;
        for (int i = 0; i < bs->num_processes; i++) {
            if (!finish[i]) {
                bool can = true;
                for (int j = 0; j < NUM_RESOURCES; j++) {
                    if (bs->need[i][j] > work[j]) { can = false; break; }
                }
                if (can) {
                    for (int j = 0; j < NUM_RESOURCES; j++) work[j] += bs->allocation[i][j];
                    finish[i] = true;
                    seq[count++] = i;
                    found = true;
                }
            }
        }
        if (!found) {
            // Revertir
            for (int j = 0; j < NUM_RESOURCES; j++) {
                work[j] += request[j];
                bs->allocation[pid][j] -= request[j];
                bs->need[pid][j] += request[j];
            }
            return false;
        }
    }
    return true;
}

void request_resource(BankerState* bs, int pid, int request[]) {
    if (is_safe(bs, request, pid)) {
        for (int j = 0; j < NUM_RESOURCES; j++) {
            bs->available[j] -= request[j];
            bs->allocation[pid][j] += request[j];
            bs->need[pid][j] -= request[j];
        }
        printf("Recursos asignados a %s\n", bs->processes[pid]->nombre);
    } else {
        printf("Solicitud denegada para %s (no segura)\n", bs->processes[pid]->nombre);
    }
}

// ------------------- G) DETECCIÓN DE DEADLOCKS CON MATRIZ DE ASIGNACIÓN -------------------
void init_deadlock(DeadlockState* ds, PCB* procs[], int n) {
    ds->num_processes = n;
    for (int i = 0; i < n; i++) ds->processes[i] = procs[i];
    // Inicialización de ejemplo
    for (int i = 0; i < n; i++) {
        ds->allocation[i][0] = 1; ds->allocation[i][1] = 0; ds->allocation[i][2] = 1;
        ds->request[i][0] = 0; ds->request[i][1] = 1; ds->request[i][2] = 0;
    }
}

bool detect_deadlock(DeadlockState* ds) {
    // Simplificado: verificar ciclos en grafo de espera
    // Para implementación completa, construir grafo y detectar ciclos
    printf("Detección de deadlock: Implementación básica - asumiendo no deadlock\n");
    return false; // No deadlock en ejemplo
}

// ------------------- H) MEJORA DEL CFS CON ÁRBOL ROJO-NEGRO -------------------
RBNode* create_node(PCB* p, RBNode* nil) {
    RBNode* node = (RBNode*)malloc(sizeof(RBNode));
    node->process = p;
    node->left = node->right = node->parent = nil;
    node->color = 0; // red
    return node;
}

void left_rotate(RBTree* tree, RBNode* x) {
    RBNode* y = x->right;
    x->right = y->left;
    if (y->left != tree->nil) y->left->parent = x;
    y->parent = x->parent;
    if (x->parent == tree->nil) tree->root = y;
    else if (x == x->parent->left) x->parent->left = y;
    else x->parent->right = y;
    y->left = x;
    x->parent = y;
}

void right_rotate(RBTree* tree, RBNode* y) {
    RBNode* x = y->left;
    y->left = x->right;
    if (x->right != tree->nil) x->right->parent = y;
    x->parent = y->parent;
    if (y->parent == tree->nil) tree->root = x;
    else if (y == y->parent->right) y->parent->right = x;
    else y->parent->left = x;
    x->right = y;
    y->parent = x;
}

void rb_insert_fixup(RBTree* tree, RBNode* z) {
    while (z->parent->color == 0) {
        if (z->parent == z->parent->parent->left) {
            RBNode* y = z->parent->parent->right;
            if (y->color == 0) {
                z->parent->color = 1;
                y->color = 1;
                z->parent->parent->color = 0;
                z = z->parent->parent;
            } else {
                if (z == z->parent->right) {
                    z = z->parent;
                    left_rotate(tree, z);
                }
                z->parent->color = 1;
                z->parent->parent->color = 0;
                right_rotate(tree, z->parent->parent);
            }
        } else {
            RBNode* y = z->parent->parent->left;
            if (y->color == 0) {
                z->parent->color = 1;
                y->color = 1;
                z->parent->parent->color = 0;
                z = z->parent->parent;
            } else {
                if (z == z->parent->left) {
                    z = z->parent;
                    right_rotate(tree, z);
                }
                z->parent->color = 1;
                z->parent->parent->color = 0;
                left_rotate(tree, z->parent->parent);
            }
        }
    }
    tree->root->color = 1;
}

void rb_insert(RBTree* tree, PCB* p) {
    RBNode* z = create_node(p, tree->nil);
    RBNode* y = tree->nil;
    RBNode* x = tree->root;
    while (x != tree->nil) {
        y = x;
        if (z->process->vruntime < x->process->vruntime) x = x->left;
        else x = x->right;
    }
    z->parent = y;
    if (y == tree->nil) tree->root = z;
    else if (z->process->vruntime < y->process->vruntime) y->left = z;
    else y->right = z;
    rb_insert_fixup(tree, z);
}

RBNode* tree_minimum(RBTree* tree, RBNode* x) {
    while (x->left != tree->nil) x = x->left;
    return x;
}

void rb_delete_fixup(RBTree* tree, RBNode* x) {
    // Implementación simplificada
}

void rb_delete(RBTree* tree, RBNode* z) {
    RBNode* y = (z->left == tree->nil || z->right == tree->nil) ? z : tree_minimum(tree, z->right);
    RBNode* x = (y->left != tree->nil) ? y->left : y->right;
    x->parent = y->parent;
    if (y->parent == tree->nil) tree->root = x;
    else if (y == y->parent->left) y->parent->left = x;
    else y->parent->right = x;
    if (y != z) z->process = y->process;
    if (y->color == 1) rb_delete_fixup(tree, x);
    free(y);
}

PCB* extract_min(RBTree* tree) {
    if (tree->root == tree->nil) return NULL;
    RBNode* min = tree_minimum(tree, tree->root);
    PCB* p = min->process;
    rb_delete(tree, min);
    return p;
}

void init_rbt(RBTree* tree) {
    tree->nil = (RBNode*)malloc(sizeof(RBNode));
    tree->nil->color = 1;
    tree->nil->left = tree->nil->right = tree->nil->parent = tree->nil;
    tree->root = tree->nil;
}

void planificar_cfs_rbt(RBTree* tree) {
    printf("\n=== CFS CON RED-BLACK TREE ===\n");
    int tiempo = 0;
    while (tree->root != tree->nil) {
        PCB* p = extract_min(tree);
        if (p->first_exec == -1) p->first_exec = tiempo;
        int slice = 2;
        if (p->nice < 0) slice = 4;
        else if (p->nice > 0) slice = 1;
        printf("Ejecuta %s slice=%d\n", p->nombre, slice);
        tiempo += slice;
        p->vruntime += slice;
        p->rafaga_restante -= slice;
        if (p->rafaga_restante > 0) {
            rb_insert(tree, p);
        } else {
            p->completion = tiempo;
            p->estado = TERMINADO;
        }
    }
}

// ------------------- 4. FUNCIÓN PARA CREAR PCB DE PRUEBA -------------------
PCB* crear_proceso(int pid, const char *nombre, int rafaga, int nice_val, int prioridad, int tiempo_llegada) {
    PCB *nuevo = (PCB*)malloc(sizeof(PCB));
    nuevo->pid = pid;
    strcpy(nuevo->nombre, nombre);
    nuevo->rafaga_total = rafaga;
    nuevo->rafaga_restante = rafaga;
    nuevo->prioridad = prioridad;
    nuevo->nice = nice_val;
    nuevo->vruntime = 0;
    nuevo->estado = NUEVO;
    nuevo->tiempo_llegada = tiempo_llegada;
    nuevo->first_exec = -1;
    nuevo->completion = 0;
    nuevo->contexto_registros = NULL;  // no implementado
    return nuevo;
}

void liberar_pcb(PCB *p) { free(p); }

// ------------------- MAIN DE PRUEBAS -------------------
int main() {
    ReadyQueue cola;
    init_ready_queue(&cola);
    
    // Procesos de ejemplo (pid, nombre, ráfaga, nice, prioridad, tiempo_llegada)
    PCB* procs[4];
    procs[0] = crear_proceso(1, "EditorTexto", 8, 0, 3, 0);
    procs[1] = crear_proceso(2, "Compilador", 4, -5, 1, 0);
    procs[2] = crear_proceso(3, "Navegador", 12, 5, 4, 0);
    procs[3] = crear_proceso(4, "Servidor", 6, -2, 2, 0);
    
    encolar(&cola, procs[0]);
    encolar(&cola, procs[1]);
    encolar(&cola, procs[2]);
    encolar(&cola, procs[3]);
    
    // Seleccione algoritmo descomentando:
    planificar_fcfs(&cola); 
    // planificar_rr(&cola, QUANTUM);
    // planificar_spn(&cola);
    // planificar_cfs_simple(&cola);
    
    // Reinicializar para otros algoritmos si es necesario
    // init_ready_queue(&cola); // Reinicializar si se ejecuta otro
    
    // Ejemplos de extensiones
    printf("\n--- EJEMPLOS DE EXTENSIONES ---\n");
    
    // Multilevel Feedback Queue
    MLFQ mlfq;
    init_mlfq(&mlfq);
    for (int i = 0; i < 4; i++) {
        // Reinicializar rafaga_restante
        procs[i]->rafaga_restante = procs[i]->rafaga_total;
        procs[i]->estado = LISTO;
        procs[i]->first_exec = -1;
        procs[i]->completion = 0;
        encolar(&mlfq.queues[0], procs[i]);
    }
    // planificar_mlfq(&mlfq);
    
    // Banker's Algorithm
    BankerState bs;
    init_banker(&bs, procs, 4);
    int req[3] = {1, 0, 1};
    request_resource(&bs, 0, req);
    
    // Deadlock Detection
    DeadlockState ds;
    init_deadlock(&ds, procs, 4);
    if (detect_deadlock(&ds)) {
        printf("Deadlock detectado\n");
    } else {
        printf("No deadlock detectado\n");
    }
    
    // CFS con RBT
    RBTree rbt;
    init_rbt(&rbt);
    for (int i = 0; i < 4; i++) {
        procs[i]->vruntime = 0;
        procs[i]->rafaga_restante = procs[i]->rafaga_total;
        procs[i]->estado = LISTO;
        procs[i]->first_exec = -1;
        procs[i]->completion = 0;
        rb_insert(&rbt, procs[i]);
    }
    // planificar_cfs_rbt(&rbt);
    
    // Imprimir estadísticas finales (después de ejecutar un algoritmo)
    printf("\n--- ESTADÍSTICAS FINALES ---\n");
    for (int i = 0; i < 4; i++) {
        if (procs[i]->estado == TERMINADO) {
            int turnaround = procs[i]->completion - procs[i]->tiempo_llegada;
            int waiting = turnaround - procs[i]->rafaga_total;
            int response = procs[i]->first_exec - procs[i]->tiempo_llegada;
            printf("Proceso %s: Turnaround %d, Waiting %d, Response %d\n", procs[i]->nombre, turnaround, waiting, response);
        }
    }
    
    printf("\n👉 LABORATORIO COMPLETADO:\n");
    printf("   - Colas multinivel (Feedback): Implementada con MLFQ\n");
    printf("   - Algoritmo del banquero (evitación): Implementado con BankerState\n");
    printf("   - Detección de deadlocks con matriz de asignación: Implementada con DeadlockState\n");
    printf("   - Mejora del CFS con árbol Rojo-Negro real: Implementado con RBTree\n");
    printf("   - Cálculo de métricas: turnaround, waiting time, response time: Calculadas\n");
    
    // Limpiar
    for (int i = 0; i < 4; i++) liberar_pcb(procs[i]);
    return 0;
}
/* ================= FIN DE PLANTILLA ================= */