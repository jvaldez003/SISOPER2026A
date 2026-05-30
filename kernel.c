/* kernel.c
   Nucleo principal del sistema
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Prototipos de files.c */
void init_filesystem();
void listar_archivos();
void crear_archivo(char nombre[]);
void abrir_archivo(char nombre[]);
void escribir_archivo(char nombre[], char texto[]);
void leer_archivo(char nombre[]);
void cerrar_archivo(char nombre[]);
void eliminar_archivo(char nombre[]);

/* Prototipos reales de memory.c y processes.c */
void memory_init(void);
void processes_init(void);

/* Wrappers con los nombres usados en este kernel */
void init_memoria()  { memory_init();    }
void init_procesos() { processes_init(); }

void mostrar_menu() {
    printf("\n==== Mini Kernel ====\n");
    printf("1. Crear archivo\n");
    printf("2. Listar archivos\n");
    printf("3. Abrir archivo\n");
    printf("4. Leer archivo\n");
    printf("5. Escribir archivo\n");
    printf("6. Cerrar archivo\n");
    printf("7. Eliminar archivo\n");
    printf("8. Salir\n");
    printf("Seleccione: ");
}

int main() {

    int opcion;
    char nombre[50];
    char texto[256];

    /* Inicializacion */
    init_memoria();
    init_procesos();
    init_filesystem();

    do {
        mostrar_menu();
        scanf("%d", &opcion);
        getchar(); /* limpiar buffer tras scanf */

        switch(opcion) {

            case 1:
                printf("Nombre archivo: ");
                fgets(nombre, sizeof(nombre), stdin);
                crear_archivo(nombre);
                break;

            case 2:
                listar_archivos();
                break;

            case 3:
                printf("Nombre archivo: ");
                fgets(nombre, sizeof(nombre), stdin);
                abrir_archivo(nombre);
                break;

            case 4:
                printf("Nombre archivo: ");
                fgets(nombre, sizeof(nombre), stdin);
                leer_archivo(nombre);
                break;

            case 5:
                printf("Nombre archivo: ");
                fgets(nombre, sizeof(nombre), stdin);
                printf("Texto: ");
                fgets(texto, sizeof(texto), stdin);
                escribir_archivo(nombre, texto);
                break;

            case 6:
                printf("Nombre archivo: ");
                fgets(nombre, sizeof(nombre), stdin);
                cerrar_archivo(nombre);
                break;

            case 7:
                printf("Nombre archivo: ");
                fgets(nombre, sizeof(nombre), stdin);
                eliminar_archivo(nombre);
                break;

            case 8:
                printf("Apagando sistema...\n");
                break;

            default:
                printf("Opcion invalida\n");
        }

    } while(opcion != 8);

    return 0;
}