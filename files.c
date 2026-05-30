/* files.c
   Simulacion basica de sistema de archivos
*/

#include <stdio.h>
#include <string.h>

#define MAX_FILES 20
#define MAX_NAME 50
#define MAX_CONTENT 256

typedef struct {
    char nombre[MAX_NAME];
    char contenido[MAX_CONTENT];
    int abierto;
    int usado;
} Archivo;

Archivo disco[MAX_FILES];

/* Inicializa estructura */
void init_filesystem() {
    int i;
    for(i = 0; i < MAX_FILES; i++) {
        disco[i].usado   = 0;
        disco[i].abierto = 0;
        memset(disco[i].nombre,   0, MAX_NAME);
        memset(disco[i].contenido, 0, MAX_CONTENT);
    }
    printf("[FS] Sistema de archivos inicializado.\n");
}

/* Crear archivo */
void crear_archivo(char nombre[]) {
    int i;
    /* Quitar salto de linea si viene de fgets */
    nombre[strcspn(nombre, "\n")] = '\0';

    /* Verificar si ya existe */
    for(i = 0; i < MAX_FILES; i++) {
        if(disco[i].usado && strcmp(disco[i].nombre, nombre) == 0) {
            printf("[FS] Error: el archivo '%s' ya existe.\n", nombre);
            return;
        }
    }
    /* Buscar espacio libre */
    for(i = 0; i < MAX_FILES; i++) {
        if(!disco[i].usado) {
            strncpy(disco[i].nombre, nombre, MAX_NAME - 1);
            disco[i].nombre[MAX_NAME - 1] = '\0';
            memset(disco[i].contenido, 0, MAX_CONTENT);
            disco[i].usado   = 1;
            disco[i].abierto = 0;
            printf("[FS] Archivo '%s' creado correctamente.\n", nombre);
            return;
        }
    }
    printf("[FS] Error: disco lleno, no se puede crear '%s'.\n", nombre);
}

/* Abrir archivo */
void abrir_archivo(char nombre[]) {
    int i;
    nombre[strcspn(nombre, "\n")] = '\0';

    for(i = 0; i < MAX_FILES; i++) {
        if(disco[i].usado && strcmp(disco[i].nombre, nombre) == 0) {
            if(disco[i].abierto) {
                printf("[FS] Aviso: '%s' ya esta abierto.\n", nombre);
            } else {
                disco[i].abierto = 1;
                printf("[FS] Archivo '%s' abierto.\n", nombre);
            }
            return;
        }
    }
    printf("[FS] Error: archivo '%s' no encontrado.\n", nombre);
}

/* Escribir archivo */
void escribir_archivo(char nombre[], char texto[]) {
    int i;
    nombre[strcspn(nombre, "\n")] = '\0';
    texto[strcspn(texto,   "\n")] = '\0';

    for(i = 0; i < MAX_FILES; i++) {
        if(disco[i].usado && strcmp(disco[i].nombre, nombre) == 0) {
            if(!disco[i].abierto) {
                printf("[FS] Error: '%s' no esta abierto. Use abrir primero.\n", nombre);
                return;
            }
            strncpy(disco[i].contenido, texto, MAX_CONTENT - 1);
            disco[i].contenido[MAX_CONTENT - 1] = '\0';
            printf("[FS] Escrito en '%s': \"%s\"\n", nombre, texto);
            return;
        }
    }
    printf("[FS] Error: archivo '%s' no encontrado.\n", nombre);
}

/* Leer archivo */
void leer_archivo(char nombre[]) {
    int i;
    nombre[strcspn(nombre, "\n")] = '\0';

    for(i = 0; i < MAX_FILES; i++) {
        if(disco[i].usado && strcmp(disco[i].nombre, nombre) == 0) {
            if(!disco[i].abierto) {
                printf("[FS] Error: '%s' no esta abierto. Use abrir primero.\n", nombre);
                return;
            }
            if(strlen(disco[i].contenido) == 0) {
                printf("[FS] Archivo '%s' esta vacio.\n", nombre);
            } else {
                printf("[FS] Contenido de '%s': \"%s\"\n",
                       nombre, disco[i].contenido);
            }
            return;
        }
    }
    printf("[FS] Error: archivo '%s' no encontrado.\n", nombre);
}

/* Cerrar archivo */
void cerrar_archivo(char nombre[]) {
    int i;
    nombre[strcspn(nombre, "\n")] = '\0';

    for(i = 0; i < MAX_FILES; i++) {
        if(disco[i].usado && strcmp(disco[i].nombre, nombre) == 0) {
            if(!disco[i].abierto) {
                printf("[FS] Aviso: '%s' ya estaba cerrado.\n", nombre);
            } else {
                disco[i].abierto = 0;
                printf("[FS] Archivo '%s' cerrado.\n", nombre);
            }
            return;
        }
    }
    printf("[FS] Error: archivo '%s' no encontrado.\n", nombre);
}

/* Eliminar archivo */
void eliminar_archivo(char nombre[]) {
    int i;
    nombre[strcspn(nombre, "\n")] = '\0';

    for(i = 0; i < MAX_FILES; i++) {
        if(disco[i].usado && strcmp(disco[i].nombre, nombre) == 0) {
            if(disco[i].abierto) {
                printf("[FS] Error: no se puede eliminar '%s', esta abierto.\n", nombre);
                return;
            }
            disco[i].usado   = 0;
            disco[i].abierto = 0;
            memset(disco[i].nombre,    0, MAX_NAME);
            memset(disco[i].contenido, 0, MAX_CONTENT);
            printf("[FS] Archivo '%s' eliminado.\n", nombre);
            return;
        }
    }
    printf("[FS] Error: archivo '%s' no encontrado.\n", nombre);
}

/* Listar archivos */
void listar_archivos() {
    int i, count = 0;

    printf("\nArchivos en sistema:\n");
    printf("%-25s %-8s\n", "Nombre", "Estado");
    printf("----------------------------------\n");

    for(i = 0; i < MAX_FILES; i++) {
        if(disco[i].usado) {
            printf("%-25s %-8s\n",
                   disco[i].nombre,
                   disco[i].abierto ? "Abierto" : "Cerrado");
            count++;
        }
    }
    if(count == 0)
        printf("  (sin archivos)\n");
    printf("----------------------------------\n");
}