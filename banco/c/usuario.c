#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "estructuras.h"
#include <poll.h>
#include <sys/msg.h>

int numero_cuenta;
int pipe_lectura;
int msgid;

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

    if(argc != 4)
    {
        fprintf(stderr, "Has hecho un uso incorrecto. Tiene que llamarse desde el banco\n");
        exit(1);

        numero_cuenta = atoi(argv[1]);
        pipe_lectura = atoi(argv[2]);
        msgid = atoi(argv[3]);
    }

    printf("\nSesión iniciada para la cuenta %d\n", numero_cuenta);

    struct pollfd fds[2];
    fds[0].fd = STDIN_FILENO; // Canal 0: El teclado
    fds[0].events = POLLIN;   // Queremos saber cuándo hay datos de entrada (IN)
    
    fds[1].fd = pipe_lectura; // Canal 1: El pipe del banco
    fds[1].events = POLLIN;

    int opcion = 0;
    mostrar_menu();
    printf("Opcion: ");
    fflush(stdout);

    while(opcion != 6)
    {
        mostrar_menu();
        printf("Opción: ");
        scanf("%d", &opcion);

        procesar_opcion(opcion);

        int ret = poll(fds, 2, -1);

        if(ret > 0)
        {
            if(fds[1].revents & POLLIN)
            {
                char alerta[256];
                int bytes_leidos = read(pipe_lectura, alerta, sizeof(alerta) - 1);
                if(bytes_leidos > 0)
                {
                    alerta[bytes_leidos] = '\0';
                    printf("\nALERTA DEL BANCO %s\n", alerta);

                    mostrar_menu();
                    printf("Opción: ");
                    fflush(stdout);
                }
            }

            if(fds[0].revents & POLLIN)
            {
                char buffer[10];
                if(fgets(buffer, sizeof(buffer), stdin) != NULL)
                {
                    opcion = atoi(buffer);

                    if(opcion > 0 && opcion <= 6)
                    {
                        procesar_opcion(opcion);
                    }

                    if(opcion != 6)
                    {
                        mostrar_menu();
                        printf("Opción: ");
                        fflush(stdout);
                    }
                }
            }
        }
    }

    //Limpieza final
    close(pipe_lectura);    

    return 0;
}