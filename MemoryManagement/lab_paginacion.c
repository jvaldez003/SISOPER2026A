/**
 * lab_paginacion.c
 * Laboratorio: Simulación de Memoria Virtual con Paginación
 *
 * Sistemas Operativos
 * Referencia: Stallings, Cap. 8 | Tanenbaum, Cap. 3.3
 *
 * Compilar: gcc -ansi -Wall -o lab_paginacion lab_paginacion.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── Constantes del sistema simulado ───────────────────── */
#define PAGE_SIZE    4096   /* Tamaño de página: 4 KB   */
#define NUM_PAGES      16   /* Páginas lógicas por proc */
#define NUM_FRAMES      8   /* Marcos físicos totales   */
#define OFFSET_BITS    12   /* log2(PAGE_SIZE)          */

/* ── Entrada de la tabla de páginas ─────────────────────
 *
 * Cada entrada (PTE - Page Table Entry) describe el estado de una
 * página lógica del proceso:
 *
 *  frame_number : En qué marco físico de RAM está cargada la página.
 *                 -1 indica que no tiene marco asignado (no presente).
 *
 *  present      : Bit de presencia (P). Si es 1, la página está en RAM.
 *                 Si es 0, está en swap (disco) o nunca fue cargada.
 *                 Un acceso con present=0 genera un PAGE FAULT.
 *
 *  dirty        : Bit de modificación (D / Modified). Se activa (1)
 *                 cuando se escribe en la página. Al expulsarla de RAM
 *                 (swap-out), si dirty=1 hay que escribirla en disco;
 *                 si dirty=0 se puede descartar sin escribir (ya está
 *                 en disco sin cambios). Ahorra I/O.
 *
 *  referenced   : Bit de referencia (R / Accessed). Se activa (1) cada
 *                 vez que se accede a la página (lectura o escritura).
 *                 Lo usa el algoritmo de reemplazo Clock/NRU para saber
 *                 qué páginas se usaron recientemente y cuáles no.
 *                 El SO lo reinicia periódicamente.
 *
 *  read_only    : Bit de protección. Si es 1, cualquier intento de
 *                 escritura genera una excepción (segmentation fault /
 *                 protection fault). Protege segmentos de código.
 *
 * Stallings, Fig. 8.8 | Tanenbaum, Fig. 3-11
 */
typedef struct {
    int frame_number;
    int present;
    int dirty;
    int referenced;
    int read_only;
} PageTableEntry;

PageTableEntry page_table[NUM_PAGES];
int            frame_map[NUM_FRAMES]; /* 1=ocupado, 0=libre */

int total_accesses = 0;
int page_faults    = 0;
int tlb_hits       = 0;

/* ── Prototipos ─────────────────────────────────────────*/
void init_page_table(void);
int  find_free_frame(void);
void handle_page_fault(int page_num);
long translate_address(long logical_addr, int write_access);
void print_page_table(void);
void print_stats(void);

/* ════════════════════════════════════════════════════════
 * init_page_table
 *
 * Inicializa la tabla de páginas poniendo todas las entradas
 * en estado "no asignado": ninguna página está en RAM,
 * sin frames, sin flags activos.
 * También limpia el mapa de marcos físicos (todos libres).
 */
void init_page_table(void) {
    int i;
    for (i = 0; i < NUM_PAGES; i++) {
        page_table[i].frame_number = -1;  /* Sin marco asignado     */
        page_table[i].present      =  0;  /* No está en RAM         */
        page_table[i].dirty        =  0;  /* Sin modificaciones     */
        page_table[i].referenced   =  0;  /* No referenciada        */
        page_table[i].read_only    =  0;  /* Lectura/escritura      */
    }
    memset(frame_map, 0, sizeof(frame_map)); /* Todos los marcos libres */
}

/* ════════════════════════════════════════════════════════
 * find_free_frame
 *
 * Busca el primer marco físico libre en el arreglo frame_map.
 * En hardware real esto lo hace el gestor de memoria del SO
 * consultando la lista de marcos libres (free-frame list).
 *
 * Retorna: índice del marco libre, o -1 si no hay ninguno.
 */
int find_free_frame(void) {
    int i;
    for (i = 0; i < NUM_FRAMES; i++) {
        if (frame_map[i] == 0) return i;
    }
    return -1;
}

/* ════════════════════════════════════════════════════════
 * handle_page_fault
 *
 * Manejador simplificado de fallo de página (page fault).
 *
 * En un SO real el flujo sería:
 *   1. El hardware detecta present=0 y lanza una excepción.
 *   2. El SO guarda el contexto del proceso.
 *   3. Busca la página en el área de swap del disco.
 *   4. Encuentra un marco libre (o desaloja una víctima).
 *   5. Carga la página del disco al marco (I/O bloqueante).
 *   6. Actualiza la entrada de la tabla de páginas.
 *   7. Restaura el contexto y reintenta la instrucción.
 *
 * PARTE 2 — Algoritmo de reemplazo Clock (Segunda Oportunidad):
 *
 * El algoritmo Clock recorre la tabla de páginas como si fuera
 * un reloj circular. Para cada página en RAM:
 *   - Si referenced == 1: se le da "segunda oportunidad",
 *     se pone referenced = 0 y se avanza.
 *   - Si referenced == 0: esta es la víctima (la menos
 *     referenciada recientemente → aproxima LRU).
 *
 * Si la víctima tiene dirty == 1 hay que escribirla en disco
 * antes de reutilizar su marco (swap-out).
 */
void handle_page_fault(int page_num) {
    int frame;
    int i;
    int victim = -1;

    page_faults++;
    printf("[PAGE FAULT] Pagina %d no esta en RAM.\n", page_num);

    frame = find_free_frame();

    if (frame == -1) {
        /*
         * ── PARTE 2: Algoritmo de reemplazo Clock ──────────────
         *
         * No hay marcos libres → debemos desalojar una página víctima.
         *
         * Paso 1: Primer barrido — buscar página con referenced == 0.
         *         Si tiene referenced == 1, poner a 0 (segunda oportunidad).
         */
        printf("[REEMPLAZO] Sin marcos libres. Ejecutando algoritmo Clock...\n");

        /* Primer barrido: dar segunda oportunidad */
        for (i = 0; i < NUM_PAGES; i++) {
            if (page_table[i].present) {
                if (page_table[i].referenced == 0) {
                    victim = i;
                    break;
                } else {
                    page_table[i].referenced = 0; /* Segunda oportunidad */
                }
            }
        }

        /* Si todos tenían referenced=1, segundo barrido (ahora todos son 0) */
        if (victim == -1) {
            for (i = 0; i < NUM_PAGES; i++) {
                if (page_table[i].present) {
                    victim = i;
                    break;
                }
            }
        }

        if (victim == -1) {
            printf("[ERROR] No se encontro victima. Estado inconsistente.\n");
            exit(1);
        }

        /*
         * Paso 2: Si la víctima está sucia (dirty == 1),
         *         simular escritura a disco (swap-out).
         *         En un SO real esto sería una operación de I/O.
         */
        if (page_table[victim].dirty == 1) {
            printf("[SWAP-OUT] Pagina %d esta sucia → escribiendo a disco.\n", victim);
        } else {
            printf("[SWAP-OUT] Pagina %d limpia → descartada sin escribir.\n", victim);
        }

        /*
         * Paso 3: Liberar el marco de la víctima y
         *         marcarla como no presente.
         */
        frame = page_table[victim].frame_number;
        frame_map[frame]                 = 0;   /* Liberar marco         */
        page_table[victim].present       = 0;   /* Ya no está en RAM     */
        page_table[victim].frame_number  = -1;  /* Sin marco asignado    */
        page_table[victim].dirty         = 0;
        page_table[victim].referenced    = 0;

        printf("[REEMPLAZO] Pagina victima: %d liberada del marco %d.\n", victim, frame);
    }

    /*
     * Cargar la nueva página en el marco libre (o recién liberado).
     * En un SO real: leer del disco. Aquí lo simulamos directamente.
     */
    frame_map[frame]                  = 1;  /* Marco ocupado          */
    page_table[page_num].frame_number = frame;
    page_table[page_num].present      = 1;  /* Ahora está en RAM      */
    page_table[page_num].referenced   = 1;  /* Recién accedida        */
    page_table[page_num].dirty        = 0;  /* Recién cargada = limpia*/
    printf("[PAGE-IN] Pagina %d → marco fisico %d.\n", page_num, frame);
}

/* ════════════════════════════════════════════════════════
 * translate_address — Traducción de dirección lógica → física
 *
 * El proceso trabaja con direcciones LÓGICAS (virtuales).
 * La MMU (Memory Management Unit) del hardware las convierte
 * a direcciones FÍSICAS en tiempo de ejecución.
 *
 * Estructura de la dirección lógica (16 bits en este sistema):
 *
 *   [  Número de página  |    Offset (desplazamiento)    ]
 *   [ bits 15..12 (4 b)  |      bits 11..0  (12 b)       ]
 *
 * - Número de página : índice en la tabla de páginas (cuál página)
 * - Offset           : posición dentro de esa página (dónde en ella)
 *
 * Dirección física = (frame_number × PAGE_SIZE) + offset
 *
 * Parámetros:
 *   logical_addr : dirección lógica de 16 bits
 *   write_access : 1 = escritura, 0 = lectura
 * Retorna: dirección física, o -1 si hay error.
 */
long translate_address(long logical_addr, int write_access) {
    int  page_num;
    int  offset;
    long physical_addr;

    total_accesses++;

    /*
     * ── PARTE 1a: Extraer número de página y desplazamiento ──────
     *
     * PAGE_SIZE = 4096 = 2^12, por lo tanto OFFSET_BITS = 12.
     *
     * Para obtener el número de página: desplazamos los bits de la
     * dirección 12 posiciones a la derecha, eliminando el offset.
     *   Ej: 0x3A4F = 0011 1010 0100 1111
     *       >> 12  = 0000 0000 0000 0011  = 3  (página 3)
     *
     * Para obtener el offset: aplicamos una máscara AND con
     * (PAGE_SIZE - 1) = 0xFFF = 0000 1111 1111 1111
     * que conserva solo los 12 bits bajos.
     *   Ej: 0x3A4F & 0x0FFF = 0xA4F = 2639 (offset dentro de la página)
     */
    page_num = (int)(logical_addr >> OFFSET_BITS);       /* Bits altos = página  */
    offset   = (int)(logical_addr &  (PAGE_SIZE - 1));   /* Bits bajos = offset  */

    printf("\n[TRADUCCION] Logica: 0x%04lX → Pag: %d, Offset: 0x%03X\n",
           logical_addr, page_num, offset);

    /* Verificar que el número de página es válido */
    if (page_num < 0 || page_num >= NUM_PAGES) {
        printf("[ERROR] Numero de pagina %d fuera de rango.\n", page_num);
        return -1;
    }

    /*
     * ── PARTE 1b: Verificar protección de escritura ──────────────
     *
     * Si la página está marcada como read_only y el acceso es de
     * escritura, el hardware lanza una excepción de protección
     * (en Linux: SIGSEGV, en Windows: Access Violation).
     * Aquí lo simulamos retornando -1.
     *
     * Caso típico: segmento de código (.text) es read_only para
     * impedir que el programa sobreescriba sus propias instrucciones.
     */
    if (write_access == 1 && page_table[page_num].read_only == 1) {
        printf("[PROTECCION] Escritura denegada en pagina %d (solo lectura).\n", page_num);
        return -1;
    }

    /*
     * Verificar si la página está en RAM (bit present).
     * Si no está → PAGE FAULT → llamar al manejador.
     * Si sí está → TLB HIT (simulado): la traducción es inmediata.
     *
     * Nota: En hardware real existe la TLB (Translation Lookaside Buffer),
     * una caché de traducciones recientes. Si la página está en la TLB,
     * la traducción es muy rápida (TLB hit). Si no, hay que consultar
     * la tabla de páginas en RAM (TLB miss, más lento). Aquí simplificamos:
     * si la página ya está presente la contamos como TLB hit.
     */
    if (!page_table[page_num].present) {
        handle_page_fault(page_num);   /* Cargar la página desde "disco" */
    } else {
        tlb_hits++;
        printf("[TLB HIT] Pagina %d → Marco %d\n",
               page_num, page_table[page_num].frame_number);
    }

    /*
     * ── PARTE 1c: Calcular dirección física y actualizar bits ─────
     *
     * Dirección física = (número de marco × tamaño de página) + offset
     *
     * Ejemplo:
     *   Página 3 → Marco físico 5
     *   Offset  = 0xA4F = 2639
     *   Dir. física = 5 × 4096 + 2639 = 20480 + 2639 = 23119 = 0x5A4F
     *
     * Luego actualizamos los bits de control:
     *   - dirty      : se activa si es escritura (la página fue modificada)
     *   - referenced : se activa siempre (la página fue accedida)
     */
    physical_addr = (long)page_table[page_num].frame_number * PAGE_SIZE + offset;

    if (write_access) {
        page_table[page_num].dirty = 1;  /* Página modificada → necesita swap-out */
    }
    page_table[page_num].referenced = 1; /* Accedida → protegida del reemplazo Clock */

    printf("[RESULTADO] Dir. fisica: 0x%05lX\n", physical_addr);
    return physical_addr;
}

/* ════════════════════════════════════════════════════════
 * print_page_table: Estado actual de la tabla de páginas. */
void print_page_table(void) {
    int i;
    printf("\n+-------+--------+---------+-------+---------+\n");
    printf("| Pag.  | Marco  |Presente | Dirty | Ref.    |\n");
    printf("+-------+--------+---------+-------+---------+\n");
    for (i = 0; i < NUM_PAGES; i++) {
        if (page_table[i].present || page_table[i].frame_number != -1) {
            printf("|  %3d  |  %3d   |    %s    |   %s   |   %s     |\n",
                i,
                page_table[i].frame_number,
                page_table[i].present    ? "S" : "N",
                page_table[i].dirty      ? "S" : "N",
                page_table[i].referenced ? "S" : "N");
        }
    }
    printf("+-------+--------+---------+-------+---------+\n");
}

void print_stats(void) {
    printf("\n=== ESTADISTICAS ===\n");
    printf("Accesos totales : %d\n", total_accesses);
    printf("Page faults     : %d (%.1f%%)\n", page_faults,
           total_accesses ? 100.0 * page_faults / total_accesses : 0.0);
    printf("TLB hits        : %d (%.1f%%)\n", tlb_hits,
           total_accesses ? 100.0 * tlb_hits / total_accesses : 0.0);
}

/* ════════════════════════════════════════════════════════
 * MAIN — Casos de prueba
 *
 * Diseño de los accesos:
 *   - Mezclamos lecturas y escrituras en páginas distintas.
 *   - Provocamos page faults (primera vez que se accede a una página).
 *   - Demostramos TLB hit (segundo acceso a la misma página).
 *   - Verificamos protección read_only (página 1).
 *   - Con los 5+ accesos adicionales llenamos los 8 marcos y
 *     forzamos el algoritmo de reemplazo Clock (Parte 2).
 */
int main(void) {
    printf("=== Laboratorio: Paginacion - Sistemas Operativos ===\n");

    init_page_table();

    /* Marcar página 1 como solo lectura (segmento de código) */
    page_table[1].read_only = 1;

    /* ── Accesos base del enunciado ─────────────────────── */
    translate_address(0x3A4F, 0); /* Lectura: pág 3, offset 0xA4F → PAGE FAULT         */
    translate_address(0x3100, 1); /* Escritura: pág 3 (ya en RAM) → TLB HIT, dirty=1   */
    translate_address(0x2050, 0); /* Lectura: pág 2, offset 0x050 → PAGE FAULT          */
    translate_address(0x1000, 1); /* Escritura pág 1 read-only → PROTECCION (error)     */

    /* ── PARTE 3: 5+ accesos adicionales ───────────────────
     *
     * Llenamos el resto de marcos (quedan 6 libres) y luego
     * provocamos un reemplazo para demostrar el algoritmo Clock.
     */

    /* Acceso 5: pág 4, lectura → PAGE FAULT, marco 2 */
    translate_address(0x4800, 0);

    /* Acceso 6: pág 5, escritura → PAGE FAULT, marco 3; dirty=1 */
    translate_address(0x5FFF, 1);

    /* Acceso 7: pág 6, lectura → PAGE FAULT, marco 4 */
    translate_address(0x6200, 0);

    /* Acceso 8: pág 7, lectura → PAGE FAULT, marco 5 */
    translate_address(0x7ABC, 0);

    /* Acceso 9: pág 0, escritura → PAGE FAULT, marco 6; dirty=1 */
    translate_address(0x0010, 1);

    /* Acceso 10: pág 8, lectura → PAGE FAULT, marco 7 (último libre) */
    translate_address(0x8500, 0);

    /* Acceso 11: pág 9, lectura → PAGE FAULT → sin marcos libres
     * → se activa el algoritmo Clock (Parte 2) para elegir víctima */
    translate_address(0x9001, 0);

    /* Acceso 12: pág 3, lectura → TLB HIT (demostración de acceso repetido) */
    translate_address(0x3050, 0);

    /* Acceso 13: pág 2, escritura → TLB HIT, se activa dirty bit */
    translate_address(0x2100, 1);

    print_page_table();
    print_stats();
    return 0;
}