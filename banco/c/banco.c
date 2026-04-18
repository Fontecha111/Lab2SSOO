#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include "estructuras.h"
#include "config.h"
#include <fcntl.h> //Para las constantes O_CREAT
#include <sys/stat.h> //Para las constantes de permisos (0644)
#include <semaphore.h> //Para usar los semáforos POSIX
#include <string.h>



int msgid;
int pipe_lectura;

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

int crear_nueva_cuenta()
{
    sem_t *sem_config = sem_open("/sem_config", 0);
    sem_t *sem_cuentas = sem_open("/sem_cuentas", 0);

    if(sem_config == SEM_FAILED || sem_cuentas == SEM_FAILED)
    {
        perror("Error abriendo semáforos en crear_nueva_cuenta");
        if(sem_config != SEM_FAILED)
        {
            sem_close(sem_config);
        }
        if(sem_cuentas != SEM_FAILED)
        {
            sem_close(sem_cuentas);
        }

        return -1;
    }

    //SECCIÓN CRÍTICA: CONFIGURACIÓN
    sem_wait(sem_config); //Bajamos la barrera

    int nuevo_id = config_banco.proximo_id;
    config_banco.proximo_id++;


    sem_post(sem_config);

    Cuenta nueva_cuenta;
    nueva_cuenta.numero_cuenta = nuevo_id;
    sprintf(nueva_cuenta.titular, "Usuario %d", nuevo_id);
    nueva_cuenta.saldo_eur = 0.0;
    nueva_cuenta.saldo_usd = 0.0;
    nueva_cuenta.saldo_gbp = 0.0;
    nueva_cuenta.num_transacciones = 0;

    sem_wait(sem_cuentas);

    FILE *archivo = fopen(config_banco.archivo_cuentas, "ab");
    if(archivo != NULL)
    {
        fwrite(&nueva_cuenta, sizeof(Cuenta), 1, archivo);
        fclose(archivo);
    }
    else
    {
        perror("Error al intentar abrir el archivo");
    }

    sem_post(sem_cuentas);

    printf("La cuenta se ha creado exitosamente. Tu número de cuenta es: %d\n", nuevo_id);

    sem_close(sem_config);
    sem_close(sem_cuentas);

    return nuevo_id;
}

void bucle_principal() {
    while (1) {
        int cuenta;

        printf("\nIntroduce número de cuenta (0 para crear nueva): ");
        if(scanf("%d", &cuenta) != 1)
        {
            int c;
            while((c = getchar()) != '\n' && c != EOF)
            {
                printf("[BANCO] Entrada invalida. Introduce un número\n");
                continue;
            }
        }

        if(cuenta == 0)
        {
            cuenta = crear_nueva_cuenta();
            if(cuenta < 0)
            {
                continue;
            }
        }

        int pipefd[2];
        if(pipe(pipefd) == -1)
        {
            perror("Error creando el pipe");
            continue;
        }

        pid_t pid = fork();
        if(pid < 0)
        {
            perror("Error creando proceso usuario");
            close(pipefd[0]);
            close(pipefd[1]);
            continue;
        }

        if (pid == 0) {
            close(pipefd[1]);

            char cuenta_str[20];
            char pipe_str[20];
            char msgid_str[20];
            sprintf(cuenta_str, "%d", cuenta);
            sprintf(pipe_str, "%d", pipefd[0]);
            sprintf(msgid_str, "%d", msgid);

            char *args[] = {"./usuario", cuenta_str, pipe_str, msgid_str, NULL};
            execv("./usuario", args);

            perror("Error ejecutando usuario");
            exit(1);
        } else {
            close(pipefd[0]); //El padre cierra el extremo de lectura del pipe

            int status;
            struct msgbuf alerta;


            while(1)
            {
                //Comprobamos si el hijo ha cerrado sesión
                pid_t result = waitpid(pid, &status, WNOHANG);
                if(result != 0)
                {
                    //Si result es > 0 (el hijo terminó) o -1 (error), salimos del bucle
                    break;
                }

                //2. Comprobamos si el monitor nos ha mandado una alerta (TIPO 2)
                //IPC_NOWAIT hace que msgrcv no se bloquee si no hay mensajes
                if(msgrcv(msgid, &alerta, sizeof(alerta.info), 2, IPC_NOWAIT) != -1)
                {
                    //Hemos recibido una alerta. Preparamos el texto a enviar al usuario
                    char mensaje_pipe[256];
                    sprintf(mensaje_pipe, "CUIDADO: Se ha detectado un mobimiento sospechoso de %.2f EUR en tu cuenta.", alerta.info.monitor.cantidad);

                    //Se lo enviamos al usuario por el pipe
                    write(pipefd[1], mensaje_pipe, strlen(mensaje_pipe));
                }

                //Dormimos para no saturar el procesador con el bucle 
                usleep(100000);
            }

            close(pipefd[1]);
        }
    }
}

int main() {

    cargar_configuracion("../c/config.txt");

    printf("La configuración se ha cargado de manera correcta\n");
    
    
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