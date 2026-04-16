#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "estructuras.h"

int numero_cuenta;
int pipe_lectura;

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
            printf("Cerrando sesión de la cuenta %d...\n", numero_cuenta);
            break;

        default:
            printf("Opcion invalida\n");
    }
}

int main(int argc, char *argv[]) {

    if(argc != 3)
    {
        fprintf(stderr, "Has hecho un uso incorrecto. Tiene que llamarse desde el banco\n");
        exit(1);

        numero_cuenta = atoi(argv[1]);
        pipe_lectura = atoi(argv[2]);
    }

    printf("\nSesión iniciada para la cuenta %d\n", numero_cuenta);
    printf("Escuchando alertas del banco en el pipe %d\n", pipe_lectura);

    int opcion = 0;

    while(opcion != 6)
    {
        mostrar_menu();
        printf("Opción: ");
        scanf("%d", &opcion);

        procesar_opcion(opcion);
    }

    close(pipe_lectura);    

    return 0;
}