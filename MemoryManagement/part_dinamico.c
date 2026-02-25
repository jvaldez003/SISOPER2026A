#include <stdio.h>
#include <stdbool.h>

#define MEMORY_SIZE 1024
#define MAX_BLOCKS 20

typedef struct {
    int start;
    int size;
    int process_id;
    bool free;
} Block;

Block blocks[MAX_BLOCKS];
int block_count = 1;

/* ─────────────────────────────────────────
   INICIALIZAR Y MOSTRAR MEMORIA
───────────────────────────────────────── */

void initialize_memory() {
    blocks[0].start = 0;
    blocks[0].size = MEMORY_SIZE;
    blocks[0].free = true;
    blocks[0].process_id = -1;
    block_count = 1;
}

void print_memory() {
    printf("\nEstado de memoria:\n");
    for (int i = 0; i < block_count; i++) {
        printf("Bloque %d | Inicio: %d | Tamanio: %d | %s\n",
               i,
               blocks[i].start,
               blocks[i].size,
               blocks[i].free ? "Libre" : "Ocupado");
    }
}

/* ─────────────────────────────────────────
   FUNCIÓN AUXILIAR: PARTIR UN BLOQUE
   Se usa igual en las 3 estrategias
───────────────────────────────────────── */

void split_and_assign(int i, int pid, int size) {
    if (blocks[i].size > size) {
        // Correr todos los bloques hacia la derecha para abrir espacio
        for (int j = block_count; j > i + 1; j--)
            blocks[j] = blocks[j - 1];

        // Crear el bloque sobrante (libre) justo después
        blocks[i + 1].start = blocks[i].start + size;
        blocks[i + 1].size  = blocks[i].size - size;
        blocks[i + 1].free  = true;
        blocks[i + 1].process_id = -1;
        block_count++;
    }
    // Asignar el bloque al proceso
    blocks[i].size = size;
    blocks[i].free = false;
    blocks[i].process_id = pid;
}

/* ─────────────────────────────────────────
   ESTRATEGIA 1 — FIRST FIT
   Asigna el PRIMER bloque libre que alcance
───────────────────────────────────────── */

void allocate_first_fit(int pid, int size) {
    for (int i = 0; i < block_count; i++) {
        if (blocks[i].free && blocks[i].size >= size) {
            split_and_assign(i, pid, size);
            return;
        }
    }
    printf("First Fit: No hay espacio suficiente para el proceso %d.\n", pid);
}

/* ─────────────────────────────────────────
   ESTRATEGIA 2 — BEST FIT
   Asigna el bloque libre MÁS PEQUEÑO que alcance
   (el que mejor se ajusta al tamaño pedido)
───────────────────────────────────────── */

void allocate_best_fit(int pid, int size) {
    int best_index = -1;
    int best_size  = MEMORY_SIZE + 1; // Empieza con un valor imposiblemente grande

    // Recorre TODOS los bloques buscando el más ajustado
    for (int i = 0; i < block_count; i++) {
        if (blocks[i].free && blocks[i].size >= size) {
            if (blocks[i].size < best_size) {
                best_size  = blocks[i].size;
                best_index = i;
            }
        }
    }

    if (best_index == -1) {
        printf("Best Fit: No hay espacio suficiente para el proceso %d.\n", pid);
        return;
    }
    split_and_assign(best_index, pid, size);
}

/* ─────────────────────────────────────────
   ESTRATEGIA 3 — WORST FIT
   Asigna el bloque libre MÁS GRANDE disponible
   (deja el sobrante más grande posible)
───────────────────────────────────────── */

void allocate_worst_fit(int pid, int size) {
    int worst_index = -1;
    int worst_size  = -1; // Empieza con un valor imposiblemente pequeño

    // Recorre TODOS los bloques buscando el más grande
    for (int i = 0; i < block_count; i++) {
        if (blocks[i].free && blocks[i].size >= size) {
            if (blocks[i].size > worst_size) {
                worst_size  = blocks[i].size;
                worst_index = i;
            }
        }
    }

    if (worst_index == -1) {
        printf("Worst Fit: No hay espacio suficiente para el proceso %d.\n", pid);
        return;
    }
    split_and_assign(worst_index, pid, size);
}

/* ─────────────────────────────────────────
   LIBERAR Y COMPACTAR (igual para las 3)
───────────────────────────────────────── */

void free_block(int pid) {
    for (int i = 0; i < block_count; i++) {
        if (blocks[i].process_id == pid) {
            blocks[i].free = true;
            blocks[i].process_id = -1;
        }
    }
}

void compact_memory() {
    int new_start = 0;
    int index = 0;
    Block temp[MAX_BLOCKS];
    int temp_count = 0;

    // Primero reubica los bloques ocupados al inicio
    for (int i = 0; i < block_count; i++) {
        if (!blocks[i].free) {
            temp[temp_count] = blocks[i];
            temp[temp_count].start = new_start;
            new_start += blocks[i].size;
            temp_count++;
        }
    }
    // Agrega un único bloque libre al final con todo el espacio restante
    temp[temp_count].start = new_start;
    temp[temp_count].size  = MEMORY_SIZE - new_start;
    temp[temp_count].free  = true;
    temp[temp_count].process_id = -1;
    temp_count++;

    // Copia el resultado de vuelta a blocks
    for (int i = 0; i < temp_count; i++)
        blocks[i] = temp[i];
    block_count = temp_count;

    printf("\nMemoria compactada.\n");
}

/* ─────────────────────────────────────────
   MAIN — PRUEBA DE LAS 3 ESTRATEGIAS
───────────────────────────────────────── */

int main() {

    /* ══════════════════════════════════
       PRUEBA 1 — FIRST FIT
    ══════════════════════════════════ */
    printf("\n========== FIRST FIT ==========");
    initialize_memory();

    allocate_first_fit(1, 200);
    allocate_first_fit(2, 300);
    allocate_first_fit(3, 100);
    print_memory();

    free_block(2);
    printf("\n-- Despues de liberar proceso 2 --");
    print_memory();

    // Nuevo proceso: First Fit toma el PRIMER hueco disponible (bloque de 300)
    allocate_first_fit(4, 150);
    printf("\n-- Despues de asignar proceso 4 (150 bytes) --");
    print_memory();

    compact_memory();
    print_memory();

    /* ══════════════════════════════════
       PRUEBA 2 — BEST FIT
    ══════════════════════════════════ */
    printf("\n========== BEST FIT ==========");
    initialize_memory();

    allocate_best_fit(1, 200);
    allocate_best_fit(2, 300);
    allocate_best_fit(3, 100);
    print_memory();

    free_block(2);
    printf("\n-- Despues de liberar proceso 2 --");
    print_memory();

    // Nuevo proceso: Best Fit toma el hueco MÁS AJUSTADO
    // Hay un hueco de 300 y uno de 424 → elige el de 300 porque ajusta mejor a 150
    allocate_best_fit(4, 150);
    printf("\n-- Despues de asignar proceso 4 (150 bytes) --");
    print_memory();

    compact_memory();
    print_memory();

    /* ══════════════════════════════════
       PRUEBA 3 — WORST FIT
    ══════════════════════════════════ */
    printf("\n========== WORST FIT ==========");
    initialize_memory();

    allocate_worst_fit(1, 200);
    allocate_worst_fit(2, 300);
    allocate_worst_fit(3, 100);
    print_memory();

    free_block(2);
    printf("\n-- Despues de liberar proceso 2 --");
    print_memory();

    // Nuevo proceso: Worst Fit toma el hueco MÁS GRANDE
    // Hay un hueco de 300 y uno de 424 → elige el de 424 porque es el más grande
    allocate_worst_fit(4, 150);
    printf("\n-- Despues de asignar proceso 4 (150 bytes) --");
    print_memory();

    compact_memory();
    print_memory();

    return 0;
}