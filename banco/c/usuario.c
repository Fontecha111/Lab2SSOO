#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "estructuras.h"

void *ejecutar_operacion(void *arg) {

    // TODO: implementar operación bancaria

    pthread_exit(NULL);
}

void mostrar_menu() {

    printf("\n--- MENU USUARIO ---\n");
    printf("1. Deposito\n");
    printf("2. Retiro\n");
    printf("3. Transferencia\n");
    printf("4. Consultar saldo\n");
    printf("5. Mover divisas\n");
    printf("6. Salir\n");
}

void procesar_opcion(int opcion) {

    pthread_t thread;

    switch(opcion) {

        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
            pthread_create(&thread, NULL, ejecutar_operacion, NULL);
            pthread_join(thread, NULL);
            break;

        case 6:
            exit(0);

        default:
            printf("Opcion invalida\n");
    }
}

int main(int argc, char *argv[]) {

    int opcion;

    while (1) {

        mostrar_menu();
        scanf("%d", &opcion);

        procesar_opcion(opcion);
    }

    return 0;
}