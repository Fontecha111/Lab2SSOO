#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include "estructuras.h"
#include "config.h"

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
        scanf("%d", &cuenta);

        int pipefd[2];
        pipe(pipefd);

        pid_t pid = fork();

        if (pid == 0) {
            close(pipefd[1]);

            char cuenta_str[20];
            sprintf(cuenta_str, "%d", cuenta);

            char *args[] = {"./usuario", cuenta_str, NULL};
            execv("./usuario", args);

            perror("Error ejecutando usuario");
            exit(1);
        } else {
            close(pipefd[0]);
        }
    }
}

int main() {

    cargar_configuracion("../c/config.txt");
    printf("Próximo ID a asignar: %d\n", config_banco.proximo_id);
    printf("Limite de transferencias (EUR): %.2f\n", config_banco.lim_transf_eur);
    printf("Umbral de retiros: %d\n", config_banco.umbral_retiros);

    msgid = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);

    if (msgid < 0) {
        perror("Error creando cola de mensajes");
        exit(1);
    }

    lanzar_monitor();

    bucle_principal();

    return 0;
}