#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include "estructuras.h"
#include "config.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>



int msgid;

void lanzar_monitor() {
    pid_t pid = fork();

    if (pid == 0) {
        char msgid_str[20];
        sprintf(msgid_str, "%d", msgid);

        char *args[] = {"./monitor", msgid_str, NULL};
        execv("./monitor", args);

        perror("Error ejecutando monitor");
        exit(1);
    }
}

void bucle_principal() {
    while (1) {
        int cuenta;

        printf("Introduce número de cuenta (0 para crear nueva): ");
        fflush(stdout);

        if (scanf("%d", &cuenta) != 1) {
            fprintf(stderr, "Entrada inválida\n");
            exit(1);
        }

        int pipefd[2];
        if (pipe(pipefd) == -1) {
            perror("Error creando pipe");
            exit(1);
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("Error en fork");
            exit(1);
        }

        if (pid == 0) {
            close(pipefd[1]);
            close(pipefd[0]);

            char cuenta_str[20];
            sprintf(cuenta_str, "%d", cuenta);

            char *args[] = {"./usuario", cuenta_str, NULL};
            execv("./usuario", args);

            perror("Error ejecutando usuario");
            exit(1);
        } else {
            close(pipefd[0]);
            close(pipefd[1]);

            waitpid(pid, NULL, 0);
        }
    }
}

int main() {

    cargar_configuracion("../c/config.txt");
    printf("La configuración se ha cargado correctamente\n");
    
    sem_unlink("/sem_cuentas");
    sem_unlink("/sem_config");

    sem_t *sem_cuentas = sem_open("/sem_cuentas", O_CREAT, 0644, 1);
    sem_t *sem_config = sem_open("/sem_config", O_CREAT, 0644, 1);

    if(sem_cuentas == SEM_FAILED || sem_config == SEM_FAILED)
    {
        perror("Error al crear los semáforos");
        exit(1);
    }

    printf("Los semáforos se han iniciado de manera correcta\n");

    msgid = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);

    if (msgid < 0) {
        perror("Error creando cola de mensajes");
        exit(1);
    }

    lanzar_monitor();

    bucle_principal();

    sem_close(sem_cuentas);
    sem_close(sem_config);
    sem_unlink("/sem_cuentas");
    sem_unlink("/sem_config");

    return 0;
}