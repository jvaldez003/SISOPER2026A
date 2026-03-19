/**
 * ============================================================
 * memory.c
 * Módulo de Gestión de Memoria — Primer Corte
 * Sistemas Operativos
 *
 * Implementa:
 *   1. Particionamiento estático (fijo)
 *   2. Particionamiento dinámico — First Fit, Best Fit, Worst Fit
 *   3. Memoria virtual con paginación
 *
 * Compilar:
 *   gcc -ansi -Wall -o memory memory.c
 *
 * Autores: <nombre(s) del grupo>
 * Fecha  : Marzo 2026
 * ============================================================
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================
 * SECCIÓN 1 — CONSTANTES GLOBALES
 * ============================================================
 *
 * MEMORY_SIZE : Tamaño total de la memoria simulada en bytes.
 * PARTITIONS  : Número de particiones fijas (particionamiento estático).
 * PART_SIZE   : Tamaño de cada partición fija = MEMORY_SIZE / PARTITIONS.
 * MAX_BLOCKS  : Máximo de bloques dinámicos posibles en memoria.
 * PAGE_SIZE   : Tamaño de cada página/marco en bytes (paginación).
 * NUM_PAGES   : Número máximo de páginas lógicas por proceso.
 * NUM_FRAMES  : Número de marcos físicos disponibles en RAM.
 * OFFSET_BITS : Bits necesarios para el offset = log2(PAGE_SIZE).
 */
#define MEMORY_SIZE  1024
#define PARTITIONS   4
#define PART_SIZE    (MEMORY_SIZE / PARTITIONS)
#define MAX_BLOCKS   20
#define PAGE_SIZE    128
#define NUM_PAGES    16
#define NUM_FRAMES   8
#define OFFSET_BITS  7   /* log2(128) = 7 */


/* ============================================================
 * SECCIÓN 2 — ESTRUCTURAS DE DATOS
 * ============================================================ */

/**
 * Partition — Partición fija de memoria (particionamiento estático).
 *
 * process_id : ID del proceso que ocupa esta partición (-1 = libre).
 * size       : Bytes que usa el proceso dentro de la partición.
 * occupied   : true si la partición está en uso, false si está libre.
 *
 * Cada partición tiene tamaño fijo PART_SIZE sin importar cuánto
 * necesite el proceso → puede haber fragmentación interna.
 */
typedef struct {
    int  process_id;
    int  size;
    bool occupied;
} Partition;

/**
 * Block — Bloque de memoria dinámica (particionamiento dinámico).
 *
 * start      : Dirección de inicio del bloque en memoria.
 * size       : Tamaño del bloque en bytes.
 * process_id : ID del proceso asignado (-1 = libre).
 * free       : true si el bloque está libre, false si está ocupado.
 *
 * Los bloques se crean del tamaño exacto que pide el proceso,
 * eliminando la fragmentación interna pero pudiendo generar
 * fragmentación externa (huecos dispersos entre bloques).
 */
typedef struct {
    int  start;
    int  size;
    int  process_id;
    bool free;
} Block;

/**
 * PageTableEntry — Entrada de la tabla de páginas (paginación).
 *
 * frame_number : Marco físico donde está cargada la página (-1 = sin marco).
 * present      : 1 si la página está en RAM, 0 si está en disco (swap).
 *                Un acceso con present=0 genera PAGE FAULT.
 * dirty        : 1 si la página fue modificada (escritura). Al sacarla
 *                de RAM se debe escribir en disco (swap-out).
 * referenced   : 1 si la página fue accedida recientemente. Lo usa el
 *                algoritmo de reemplazo Clock para elegir víctima.
 * read_only    : 1 si la página es solo lectura (ej: código). Un intento
 *                de escritura genera excepción de protección.
 */
typedef struct {
    int frame_number;
    int present;
    int dirty;
    int referenced;
    int read_only;
} PageTableEntry;


/* ============================================================
 * SECCIÓN 3 — VARIABLES GLOBALES
 * ============================================================ */

/* -- Particionamiento estático -- */
Partition memory[PARTITIONS];

/* -- Particionamiento dinámico -- */
Block blocks[MAX_BLOCKS];
int   block_count = 1;

/* -- Paginación -- */
PageTableEntry page_table[NUM_PAGES];
int            frame_map[NUM_FRAMES];  /* 1 = ocupado, 0 = libre */
int            total_accesses = 0;
int            page_faults    = 0;
int            tlb_hits       = 0;


/* ============================================================
 * SECCIÓN 4 — PARTICIONAMIENTO ESTÁTICO (FIJO)
 * ============================================================ */

/**
 * initalize_mem_static
 * Inicializa las PARTITIONS particiones fijas como libres.
 * Pone process_id = -1, size = 0, occupied = false en cada una.
 */
void initalize_mem_static() {
    int i;
    for (i = 0; i < PARTITIONS; i++) {
        memory[i].process_id = -1;
        memory[i].size       = 0;
        memory[i].occupied   = false;
    }
}

/**
 * print_mem_static
 * Recorre las PARTITIONS particiones e imprime su estado:
 * si está ocupada muestra el proceso y los bytes usados,
 * si está libre lo indica. También muestra la fragmentación
 * interna de cada partición ocupada.
 */
void print_mem_static() {
    int i;
    printf("\n--- Estado Particionamiento Estatico ---\n");
    for (i = 0; i < PARTITIONS; i++) {
        if (memory[i].occupied) {
            printf("Particion %d | Proceso %d | %d/%d bytes usados | Fragmentacion interna: %d bytes\n",
                i, memory[i].process_id, memory[i].size,
                PART_SIZE, PART_SIZE - memory[i].size);
        } else {
            printf("Particion %d | LIBRE | %d bytes disponibles\n", i, PART_SIZE);
        }
    }
}

/**
 * allocate_mem_static
 * Asigna una partición al proceso pid con el tamaño size.
 * Si size > PART_SIZE, rechaza la solicitud (no cabe).
 * Busca secuencialmente la primera partición libre y la asigna.
 * Estrategia implícita: First Fit (primera libre disponible).
 *
 * @param pid  ID del proceso a asignar.
 * @param size Bytes requeridos por el proceso.
 */
void allocate_mem_static(int pid, int size) {
    int i;
    if (size > PART_SIZE) {
        printf("  [ERROR] El proceso %d pide %d bytes pero cada particion es de %d bytes.\n",
               pid, size, PART_SIZE);
        return;
    }
    for (i = 0; i < PARTITIONS; i++) {
        if (!memory[i].occupied) {
            memory[i].process_id = pid;
            memory[i].size       = size;
            memory[i].occupied   = true;
            printf("  [OK] Proceso %d asignado a Particion %d (%d bytes).\n", pid, i, size);
            return;
        }
    }
    printf("  [ERROR] No hay particiones libres para el proceso %d.\n", pid);
}

/**
 * free_mem_static
 * Libera la partición ocupada por el proceso pid.
 * Recorre las particiones hasta encontrar la que tiene ese pid.
 *
 * @param pid ID del proceso a liberar.
 */
void free_mem_static(int pid) {
    int i;
    for (i = 0; i < PARTITIONS; i++) {
        if (memory[i].process_id == pid) {
            memory[i].process_id = -1;
            memory[i].size       = 0;
            memory[i].occupied   = false;
            printf("  [OK] Proceso %d liberado de Particion %d.\n", pid, i);
            return;
        }
    }
    printf("  [ERROR] Proceso %d no encontrado en memoria.\n", pid);
}


/* ============================================================
 * SECCIÓN 5 — PARTICIONAMIENTO DINÁMICO
 * ============================================================ */

/**
 * initialize_memory_dynamic
 * Inicializa la memoria dinámica como un único bloque libre
 * de MEMORY_SIZE bytes. block_count se reinicia a 1.
 */
void initialize_memory_dynamic() {
    blocks[0].start      = 0;
    blocks[0].size       = MEMORY_SIZE;
    blocks[0].free       = true;
    blocks[0].process_id = -1;
    block_count = 1;
}

/**
 * print_memory_dynamic
 * Muestra el estado de todos los bloques dinámicos:
 * su índice, dirección de inicio, tamaño y si está libre u ocupado.
 */
void print_memory_dynamic() {
    int i;
    printf("\n--- Estado Particionamiento Dinamico ---\n");
    for (i = 0; i < block_count; i++) {
        printf("Bloque %d | Inicio: %4d | Tamanio: %4d | %s",
               i, blocks[i].start, blocks[i].size,
               blocks[i].free ? "Libre" : "Ocupado");
        if (!blocks[i].free)
            printf(" (Proceso %d)", blocks[i].process_id);
        printf("\n");
    }
}

/**
 * split_and_assign
 * Función auxiliar compartida por las tres estrategias.
 * Si el bloque i es más grande que size, lo parte en dos:
 *   - El primer bloque (tamaño = size) se asigna al proceso.
 *   - El segundo bloque (sobrante) queda libre.
 * Para insertar el bloque sobrante, corre todos los bloques
 * posteriores una posición hacia la derecha.
 *
 * @param i    Índice del bloque a partir y asignar.
 * @param pid  ID del proceso.
 * @param size Bytes requeridos.
 */
void split_and_assign(int i, int pid, int size) {
    int j;
    if (blocks[i].size > size) {
        for (j = block_count; j > i + 1; j--)
            blocks[j] = blocks[j - 1];

        blocks[i + 1].start      = blocks[i].start + size;
        blocks[i + 1].size       = blocks[i].size - size;
        blocks[i + 1].free       = true;
        blocks[i + 1].process_id = -1;
        block_count++;
    }
    blocks[i].size       = size;
    blocks[i].free       = false;
    blocks[i].process_id = pid;
}

/**
 * allocate_first_fit
 * Estrategia First Fit: asigna el PRIMER bloque libre que
 * tenga tamaño suficiente. Es la más rápida pero puede generar
 * más fragmentación externa que Best Fit.
 *
 * @param pid  ID del proceso.
 * @param size Bytes requeridos.
 */
void allocate_first_fit(int pid, int size) {
    int i;
    for (i = 0; i < block_count; i++) {
        if (blocks[i].free && blocks[i].size >= size) {
            split_and_assign(i, pid, size);
            printf("  [First Fit] Proceso %d → Bloque %d (inicio %d, %d bytes).\n",
                   pid, i, blocks[i].start, size);
            return;
        }
    }
    printf("  [First Fit] Sin espacio para el proceso %d.\n", pid);
}

/**
 * allocate_best_fit
 * Estrategia Best Fit: recorre TODOS los bloques libres y elige
 * el más pequeño que aún sea suficiente para el proceso.
 * Minimiza el desperdicio interno del bloque elegido, pero puede
 * generar muchos huecos pequeños inutilizables.
 *
 * @param pid  ID del proceso.
 * @param size Bytes requeridos.
 */
void allocate_best_fit(int pid, int size) {
    int i;
    int best_index = -1;
    int best_size  = MEMORY_SIZE + 1;

    for (i = 0; i < block_count; i++) {
        if (blocks[i].free && blocks[i].size >= size) {
            if (blocks[i].size < best_size) {
                best_size  = blocks[i].size;
                best_index = i;
            }
        }
    }
    if (best_index == -1) {
        printf("  [Best Fit] Sin espacio para el proceso %d.\n", pid);
        return;
    }
    split_and_assign(best_index, pid, size);
    printf("  [Best Fit] Proceso %d → Bloque %d (inicio %d, %d bytes).\n",
           pid, best_index, blocks[best_index].start, size);
}

/**
 * allocate_worst_fit
 * Estrategia Worst Fit: recorre TODOS los bloques libres y elige
 * el MÁS GRANDE. La idea es que el sobrante después de partir
 * también sea grande y útil para futuros procesos.
 * Puede ser ineficiente si los procesos son de tamaño similar.
 *
 * @param pid  ID del proceso.
 * @param size Bytes requeridos.
 */
void allocate_worst_fit(int pid, int size) {
    int i;
    int worst_index = -1;
    int worst_size  = -1;

    for (i = 0; i < block_count; i++) {
        if (blocks[i].free && blocks[i].size >= size) {
            if (blocks[i].size > worst_size) {
                worst_size  = blocks[i].size;
                worst_index = i;
            }
        }
    }
    if (worst_index == -1) {
        printf("  [Worst Fit] Sin espacio para el proceso %d.\n", pid);
        return;
    }
    split_and_assign(worst_index, pid, size);
    printf("  [Worst Fit] Proceso %d → Bloque %d (inicio %d, %d bytes).\n",
           pid, worst_index, blocks[worst_index].start, size);
}

/**
 * free_block_dynamic
 * Libera el bloque del proceso pid: lo marca como libre
 * y elimina su process_id. No fusiona bloques adyacentes
 * (la compactación se hace explícitamente con compact_memory).
 *
 * @param pid ID del proceso a liberar.
 */
void free_block_dynamic(int pid) {
    int i;
    for (i = 0; i < block_count; i++) {
        if (blocks[i].process_id == pid) {
            blocks[i].free       = true;
            blocks[i].process_id = -1;
            printf("  [OK] Proceso %d liberado (Bloque %d).\n", pid, i);
            return;
        }
    }
    printf("  [ERROR] Proceso %d no encontrado.\n", pid);
}

/**
 * compact_memory
 * Compactación de memoria dinámica: mueve todos los bloques
 * ocupados hacia el inicio de la memoria, eliminando los huecos
 * entre ellos. Al final crea un único bloque libre consolidado
 * con todo el espacio disponible.
 * Resuelve la fragmentación externa generada por liberaciones.
 */
void compact_memory() {
    int i;
    int new_start  = 0;
    int temp_count = 0;
    Block temp[MAX_BLOCKS];

    for (i = 0; i < block_count; i++) {
        if (!blocks[i].free) {
            temp[temp_count]       = blocks[i];
            temp[temp_count].start = new_start;
            new_start += blocks[i].size;
            temp_count++;
        }
    }
    temp[temp_count].start      = new_start;
    temp[temp_count].size       = MEMORY_SIZE - new_start;
    temp[temp_count].free       = true;
    temp[temp_count].process_id = -1;
    temp_count++;

    for (i = 0; i < temp_count; i++)
        blocks[i] = temp[i];
    block_count = temp_count;

    printf("  [OK] Memoria compactada. Espacio libre consolidado: %d bytes desde %d.\n",
           MEMORY_SIZE - new_start, new_start);
}


/* ============================================================
 * SECCIÓN 6 — MEMORIA VIRTUAL CON PAGINACIÓN
 * ============================================================ */

/**
 * init_page_table
 * Inicializa la tabla de páginas: todas las páginas quedan
 * sin marco asignado (frame_number = -1), no presentes en RAM,
 * sin modificaciones ni referencias. Todos los marcos quedan libres.
 */
void init_page_table() {
    int i;
    for (i = 0; i < NUM_PAGES; i++) {
        page_table[i].frame_number = -1;
        page_table[i].present      =  0;
        page_table[i].dirty        =  0;
        page_table[i].referenced   =  0;
        page_table[i].read_only    =  0;
    }
    memset(frame_map, 0, sizeof(frame_map));
    total_accesses = 0;
    page_faults    = 0;
    tlb_hits       = 0;
}

/**
 * find_free_frame
 * Busca el primer marco físico libre en frame_map.
 * En un SO real esto consulta la lista de marcos libres del kernel.
 *
 * @return Índice del marco libre, o -1 si no hay ninguno.
 */
int find_free_frame() {
    int i;
    for (i = 0; i < NUM_FRAMES; i++) {
        if (frame_map[i] == 0) return i;
    }
    return -1;
}

/**
 * handle_page_fault
 * Manejador de fallo de página (Page Fault).
 * Si hay un marco libre, carga la página ahí directamente.
 * Si no hay marcos libres, aplica el algoritmo de reemplazo Clock:
 *   - Primer barrido: da segunda oportunidad a páginas con referenced=1
 *     (les pone referenced=0) y elige la primera con referenced=0.
 *   - Segundo barrido: si todas tenían referenced=1, ahora todas son 0,
 *     elige la primera página presente.
 *   - Si la víctima tiene dirty=1, simula escritura a disco (swap-out).
 *   - Libera el marco de la víctima y carga la nueva página.
 *
 * @param page_num Número de página que generó el fallo.
 */
void handle_page_fault(int page_num) {
    int frame;
    int i;
    int victim = -1;

    page_faults++;
    printf("  [PAGE FAULT] Pagina %d no esta en RAM.\n", page_num);

    frame = find_free_frame();

    if (frame == -1) {
        printf("  [REEMPLAZO] Sin marcos libres. Ejecutando algoritmo Clock...\n");

        /* Primer barrido: segunda oportunidad */
        for (i = 0; i < NUM_PAGES; i++) {
            if (page_table[i].present) {
                if (page_table[i].referenced == 0) {
                    victim = i;
                    break;
                } else {
                    page_table[i].referenced = 0;
                }
            }
        }
        /* Segundo barrido si todos tenían referenced=1 */
        if (victim == -1) {
            for (i = 0; i < NUM_PAGES; i++) {
                if (page_table[i].present) {
                    victim = i;
                    break;
                }
            }
        }
        if (victim == -1) {
            printf("  [ERROR] No se encontro victima.\n");
            return;
        }

        if (page_table[victim].dirty == 1)
            printf("  [SWAP-OUT] Pagina %d sucia → escribiendo a disco.\n", victim);
        else
            printf("  [SWAP-OUT] Pagina %d limpia → descartada sin escribir.\n", victim);

        frame = page_table[victim].frame_number;
        frame_map[frame]                = 0;
        page_table[victim].present      = 0;
        page_table[victim].frame_number = -1;
        page_table[victim].dirty        = 0;
        page_table[victim].referenced   = 0;
        printf("  [REEMPLAZO] Victima: pagina %d liberada del marco %d.\n", victim, frame);
    }

    frame_map[frame]                   = 1;
    page_table[page_num].frame_number  = frame;
    page_table[page_num].present       = 1;
    page_table[page_num].referenced    = 1;
    page_table[page_num].dirty         = 0;
    printf("  [PAGE-IN] Pagina %d → marco fisico %d.\n", page_num, frame);
}

/**
 * translate_address
 * Traduce una dirección lógica a una dirección física usando la tabla
 * de páginas. La MMU extrae el número de página y el offset de la
 * dirección lógica mediante operaciones de bits:
 *
 *   numero_de_pagina = direccion_logica >> OFFSET_BITS
 *   offset           = direccion_logica &  (PAGE_SIZE - 1)
 *   direccion_fisica = frame_number * PAGE_SIZE + offset
 *
 * Si la página no está en RAM genera PAGE FAULT. Si está, cuenta
 * como TLB HIT (acceso rápido). Actualiza los bits dirty y referenced.
 *
 * @param logical_addr Dirección lógica del proceso.
 * @param write_access 1 = escritura, 0 = lectura.
 * @return Dirección física calculada, o -1 si hay error.
 */
long translate_address(long logical_addr, int write_access) {
    int  page_num;
    int  offset;
    long physical_addr;

    total_accesses++;

    page_num = (int)(logical_addr >> OFFSET_BITS);
    offset   = (int)(logical_addr &  (PAGE_SIZE - 1));

    printf("\n  [TRADUCCION] Logica: 0x%04lX → Pagina: %d, Offset: %d\n",
           logical_addr, page_num, offset);

    if (page_num < 0 || page_num >= NUM_PAGES) {
        printf("  [ERROR] Numero de pagina %d fuera de rango.\n", page_num);
        return -1;
    }
    if (write_access == 1 && page_table[page_num].read_only == 1) {
        printf("  [PROTECCION] Escritura denegada en pagina %d (solo lectura).\n", page_num);
        return -1;
    }

    if (!page_table[page_num].present) {
        handle_page_fault(page_num);
    } else {
        tlb_hits++;
        printf("  [TLB HIT] Pagina %d → Marco %d\n",
               page_num, page_table[page_num].frame_number);
    }

    physical_addr = (long)page_table[page_num].frame_number * PAGE_SIZE + offset;

    if (write_access)
        page_table[page_num].dirty = 1;
    page_table[page_num].referenced = 1;

    printf("  [RESULTADO] Direccion fisica: 0x%04lX (%ld)\n", physical_addr, physical_addr);
    return physical_addr;
}

/**
 * print_page_table
 * Imprime la tabla de páginas mostrando solo las entradas que
 * han sido usadas (presentes o con marco asignado anteriormente).
 * Muestra: número de página, marco físico, bits presente/dirty/referenced.
 */
void print_page_table() {
    int i;
    printf("\n+--------+--------+----------+-------+-----------+\n");
    printf("| Pagina | Marco  | Presente | Dirty | Referencia|\n");
    printf("+--------+--------+----------+-------+-----------+\n");
    for (i = 0; i < NUM_PAGES; i++) {
        if (page_table[i].present || page_table[i].frame_number != -1) {
            printf("|  %3d   |  %3d   |    %s     |   %s   |     %s     |\n",
                i,
                page_table[i].frame_number,
                page_table[i].present    ? "S" : "N",
                page_table[i].dirty      ? "S" : "N",
                page_table[i].referenced ? "S" : "N");
        }
    }
    printf("+--------+--------+----------+-------+-----------+\n");
}

/**
 * print_stats_paging
 * Imprime estadísticas del módulo de paginación:
 * total de accesos, page faults y TLB hits con porcentajes.
 */
void print_stats_paging() {
    printf("\n=== Estadisticas de Paginacion ===\n");
    printf("Accesos totales : %d\n", total_accesses);
    printf("Page faults     : %d (%.1f%%)\n", page_faults,
           total_accesses ? 100.0 * page_faults / total_accesses : 0.0);
    printf("TLB hits        : %d (%.1f%%)\n", tlb_hits,
           total_accesses ? 100.0 * tlb_hits / total_accesses : 0.0);
}


/* ============================================================
 * SECCIÓN 7 — MENÚS DE DEMOSTRACIÓN
 * ============================================================ */

/**
 * demo_static
 * Prueba del particionamiento estático:
 * inicializa memoria, asigna 3 procesos, imprime, libera uno e imprime.
 */
void demo_static() {
    printf("\n========== PARTICIONAMIENTO ESTATICO ==========\n");
    printf("Memoria total: %d bytes | %d particiones de %d bytes c/u\n",
           MEMORY_SIZE, PARTITIONS, PART_SIZE);

    initalize_mem_static();
    allocate_mem_static(1, 100);
    allocate_mem_static(2, 200);
    allocate_mem_static(3, 50);
    print_mem_static();

    printf("\n-- Liberando proceso 2 --\n");
    free_mem_static(2);
    print_mem_static();
}

/**
 * demo_dynamic
 * Prueba comparativa de las tres estrategias dinámicas.
 * Para cada una: asigna 3 procesos, libera el del medio,
 * asigna un 4to proceso (aquí se ve la diferencia entre estrategias)
 * y compacta la memoria al final.
 */
void demo_dynamic() {
    printf("\n========== PARTICIONAMIENTO DINAMICO ==========\n");

    /* ── First Fit ── */
    printf("\n--- FIRST FIT ---\n");
    initialize_memory_dynamic();
    allocate_first_fit(1, 200);
    allocate_first_fit(2, 300);
    allocate_first_fit(3, 100);
    print_memory_dynamic();
    printf("\n-- Liberando proceso 2 --\n");
    free_block_dynamic(2);
    print_memory_dynamic();
    printf("\n-- Asignando proceso 4 (150 bytes) --\n");
    printf("   First Fit toma el PRIMER hueco suficiente (bloque de 300)\n");
    allocate_first_fit(4, 150);
    print_memory_dynamic();
    compact_memory();
    print_memory_dynamic();

    /* ── Best Fit ── */
    printf("\n--- BEST FIT ---\n");
    initialize_memory_dynamic();
    allocate_best_fit(1, 200);
    allocate_best_fit(2, 300);
    allocate_best_fit(3, 100);
    print_memory_dynamic();
    printf("\n-- Liberando proceso 2 --\n");
    free_block_dynamic(2);
    print_memory_dynamic();
    printf("\n-- Asignando proceso 4 (150 bytes) --\n");
    printf("   Best Fit: huecos disponibles: 300 y 524 → elige 300 (mas ajustado)\n");
    allocate_best_fit(4, 150);
    print_memory_dynamic();
    compact_memory();
    print_memory_dynamic();

    /* ── Worst Fit ── */
    printf("\n--- WORST FIT ---\n");
    initialize_memory_dynamic();
    allocate_worst_fit(1, 200);
    allocate_worst_fit(2, 300);
    allocate_worst_fit(3, 100);
    print_memory_dynamic();
    printf("\n-- Liberando proceso 2 --\n");
    free_block_dynamic(2);
    print_memory_dynamic();
    printf("\n-- Asignando proceso 4 (150 bytes) --\n");
    printf("   Worst Fit: huecos disponibles: 300 y 524 → elige 524 (el mas grande)\n");
    allocate_worst_fit(4, 150);
    print_memory_dynamic();
    compact_memory();
    print_memory_dynamic();
}

/**
 * demo_paging
 * Prueba del módulo de paginación:
 * Realiza una serie de accesos a direcciones lógicas que demuestran
 * page faults, TLB hits, protección de escritura y reemplazo Clock.
 */
void demo_paging() {
    printf("\n========== MEMORIA VIRTUAL CON PAGINACION ==========\n");
    printf("PAGE_SIZE=%d bytes | NUM_FRAMES=%d marcos | NUM_PAGES=%d paginas logicas\n",
           PAGE_SIZE, NUM_FRAMES, NUM_PAGES);

    init_page_table();
    page_table[1].read_only = 1;  /* Pagina 1 = segmento de codigo (solo lectura) */

    printf("\n-- Acceso 1: lectura pagina 3 → PAGE FAULT --\n");
    translate_address(0x01AF, 0);

    printf("\n-- Acceso 2: escritura pagina 3 (ya en RAM) → TLB HIT, dirty=1 --\n");
    translate_address(0x0180, 1);

    printf("\n-- Acceso 3: lectura pagina 2 → PAGE FAULT --\n");
    translate_address(0x0150, 0);

    printf("\n-- Acceso 4: escritura pagina 1 (solo lectura) → PROTECCION --\n");
    translate_address(0x0080, 1);

    printf("\n-- Accesos 5 al 9: llenando marcos --\n");
    translate_address(0x0200, 0);
    translate_address(0x0280, 1);
    translate_address(0x0300, 0);
    translate_address(0x0380, 0);
    translate_address(0x0010, 1);

    printf("\n-- Acceso 10: llena el ultimo marco libre --\n");
    translate_address(0x0400, 0);

    printf("\n-- Acceso 11: nueva pagina → sin marcos → algoritmo Clock --\n");
    translate_address(0x0480, 0);

    printf("\n-- Acceso 12: pagina 3 de nuevo → TLB HIT --\n");
    translate_address(0x01B0, 0);

    print_page_table();
    print_stats_paging();
}

/**
 * menu_static / menu_dynamic / menu_paging
 * Submenús interactivos para cada módulo: permiten al usuario
 * ingresar datos propios (pid, size, dirección) y probar
 * las funciones en tiempo real.
 */
void menu_static() {
    int opt, pid, size;
    initalize_mem_static();
    do {
        printf("\n--- Menu Particionamiento Estatico ---\n");
        printf("1. Asignar proceso\n");
        printf("2. Liberar proceso\n");
        printf("3. Ver estado\n");
        printf("4. Reiniciar memoria\n");
        printf("0. Volver\n");
        printf("Opcion: "); scanf("%d", &opt);
        switch(opt) {
            case 1:
                printf("PID: "); scanf("%d", &pid);
                printf("Tamanio (max %d): ", PART_SIZE); scanf("%d", &size);
                allocate_mem_static(pid, size);
                break;
            case 2:
                printf("PID a liberar: "); scanf("%d", &pid);
                free_mem_static(pid);
                break;
            case 3: print_mem_static(); break;
            case 4: initalize_mem_static(); printf("  Memoria reiniciada.\n"); break;
        }
    } while(opt != 0);
}

void menu_dynamic() {
    int opt, pid, size, strat;
    initialize_memory_dynamic();
    do {
        printf("\n--- Menu Particionamiento Dinamico ---\n");
        printf("1. Asignar proceso\n");
        printf("2. Liberar proceso\n");
        printf("3. Compactar memoria\n");
        printf("4. Ver estado\n");
        printf("5. Reiniciar memoria\n");
        printf("0. Volver\n");
        printf("Opcion: "); scanf("%d", &opt);
        switch(opt) {
            case 1:
                printf("PID: "); scanf("%d", &pid);
                printf("Tamanio: "); scanf("%d", &size);
                printf("Estrategia (1=First 2=Best 3=Worst): "); scanf("%d", &strat);
                if (strat == 1) allocate_first_fit(pid, size);
                else if (strat == 2) allocate_best_fit(pid, size);
                else allocate_worst_fit(pid, size);
                break;
            case 2:
                printf("PID a liberar: "); scanf("%d", &pid);
                free_block_dynamic(pid);
                break;
            case 3: compact_memory(); break;
            case 4: print_memory_dynamic(); break;
            case 5: initialize_memory_dynamic(); printf("  Memoria reiniciada.\n"); break;
        }
    } while(opt != 0);
}

void menu_paging() {
    int opt, write;
    long addr;
    init_page_table();
    do {
        printf("\n--- Menu Paginacion ---\n");
        printf("1. Traducir direccion logica\n");
        printf("2. Ver tabla de paginas\n");
        printf("3. Ver estadisticas\n");
        printf("4. Reiniciar paginacion\n");
        printf("0. Volver\n");
        printf("Opcion: "); scanf("%d", &opt);
        switch(opt) {
            case 1:
                printf("Direccion logica (decimal): "); scanf("%ld", &addr);
                printf("Tipo (0=lectura, 1=escritura): "); scanf("%d", &write);
                translate_address(addr, write);
                break;
            case 2: print_page_table(); break;
            case 3: print_stats_paging(); break;
            case 4: init_page_table(); printf("  Paginacion reiniciada.\n"); break;
        }
    } while(opt != 0);
}


/* ============================================================
 * MAIN — Menú principal
 * ============================================================ */
int main() {
    int opt;
    printf("=================================================\n");
    printf("  Modulo de Gestion de Memoria — Sistemas Op.\n");
    printf("=================================================\n");
    do {
        printf("\n===== MENU PRINCIPAL =====\n");
        printf("1. Demo Particionamiento Estatico\n");
        printf("2. Demo Particionamiento Dinamico (First/Best/Worst Fit)\n");
        printf("3. Demo Memoria Virtual con Paginacion\n");
        printf("4. Interactivo — Particionamiento Estatico\n");
        printf("5. Interactivo — Particionamiento Dinamico\n");
        printf("6. Interactivo — Paginacion\n");
        printf("0. Salir\n");
        printf("Opcion: "); scanf("%d", &opt);

        switch(opt) {
            case 1: demo_static();   break;
            case 2: demo_dynamic();  break;
            case 3: demo_paging();   break;
            case 4: menu_static();   break;
            case 5: menu_dynamic();  break;
            case 6: menu_paging();   break;
            case 0: printf("Saliendo...\n"); break;
            default: printf("Opcion invalida.\n");
        }
    } while(opt != 0);

    return 0;
}