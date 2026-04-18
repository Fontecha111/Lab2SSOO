#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include "estructuras.h"
#include <poll.h>
#include <sys/msg.h>
#include <semaphore.h>
#include "config.h"

int numero_cuenta;
int pipe_lectura;
int msgid;

static const char *nom_divisa(int divisa)
{
    if(divisa == 1)
    {
        return "EUR";
    }

    if(divisa == 2)
    {
        return "USD";
    }

    if(divisa == 3)
    {
        return "GBP";
    }

    return "DESCONOCIDA";
}

static int leer_divisa_deposito(int *divisa)
{
    char buffer[128];
    char *endptr;
    long valor;

    printf("\nPor favor, selecciona la divisa que quieres usar en el depósito: \n");
    printf("1. EUR\n");
    printf("2. USD\n");
    printf("3. GBP\n");
    printf("Divisa: ");
    fflush(stdout);

    if(fgets(buffer, sizeof(buffer), stdin) == NULL)
    {
        return 0;
    }

    errno = 0;
    valor = strtol(buffer, &endptr, 10);

    if(endptr == buffer)
    {
        return 0;
    }

    while(*endptr == ' ' || *endptr == '\t')
    {
        endptr++;
    }

    if(*endptr != '\n' && *endptr != '\0')
    {
        return 0;
    }

    if(errno == ERANGE || valor < 1 || valor > 3)
    {
        return 0;
    }

    *divisa = (int)valor;
    return 1;
}

static int leer_cantidad_deposito(float *cantidad, int divisa)
{
    char buffer[123];
    char *endptr;

    printf("\nPor favor, introduce la cantidad que quieres depositar (%s): ", nom_divisa(divisa));
    fflush(stdout);

    if(fgets(buffer, sizeof(buffer), stdin) == NULL)
    {
        return 0;
    }

    errno = 0;
    *cantidad = strtof(buffer, &endptr);

    if(endptr == buffer)
    {
        return 0;
    }

    while(*endptr == ' ' || *endptr == '\t')
    {
        endptr++;
    }

    if(*endptr != '\n' && *endptr != '\0')
    {
        return 0;
    }

    if(errno == ERANGE || !isfinite(*cantidad) || *cantidad <= 0.0f)
    {
        return 0;
    }

    return 1;
}



void *ejecutar_operacion(void *arg) {

    // TODO: implementar operación bancaria
    int tipo_op = *(int*)arg;
    free(arg);

    sem_t *sem_cuentas = sem_open("/sem_cuentas", 0);
    if(sem_cuentas == SEM_FAILED)
    {
        perror("Error abriendo semáforo en el hilo");
        pthread_exit(NULL);
    }

    if(tipo_op == 4)
    {
        Cuenta c;
        sem_wait(sem_cuentas);

        FILE *f = fopen("../c/config.txt", "rb");
        f = fopen("cuentas.dat", "rb");

        if(f != NULL)
        {
            while(fread(&c, sizeof(Cuenta), 1, f))
            {
                if(c.numero_cuenta == numero_cuenta)
                {
                    printf("\n[SALDO ACTUAL] EUR: %.2f | USD: %.2f | GBP: %.2f\n", c.saldo_eur, c.saldo_usd, c.saldo_gbp);
                    break;
                }
            }
            fclose(f);
        }
        sem_post(sem_cuentas); //Barrera se levanta
    }
    else if(tipo_op == 1)
    {
        //OPCIÓN 1: DEPOSITAR
        int divisa;
        float cantidad;
        if(!leer_divisa_deposito(&divisa))
        {
            printf("[ERROR] La divisa introducida no es válida. Elige 1 (EUR), 2 (USD) o 3 (GBP)\n");
            sem_close(sem_cuentas);
            pthread_exit(NULL);
            
        }

        if(!leer_cantidad_deposito(&cantidad, divisa))
        {
            printf("[ERROR] La cantidad introducida no es válida. Introduce un número positivo y válido");
            sem_close(sem_cuentas);
            pthread_exit(NULL);
        }


        Cuenta c;
        int cuenta_encontrada = 0;

        sem_wait(sem_cuentas); //Barrera bajada



        FILE *f = fopen("cuentas.dat", "r+b");
        if(f != NULL)
        {
            while(fread(&c, sizeof(Cuenta), 1, f))
            {
                if(c.numero_cuenta == numero_cuenta)
                {
                    cuenta_encontrada = 1;
                    
                    //Modificamos el saldo según la divisa elegida
                    if(divisa == 1)
                    {
                        c.saldo_eur += cantidad;
                    }
                    else if(divisa == 2)
                    {
                        c.saldo_usd += cantidad;
                    }
                    else 
                    {
                        c.saldo_gbp += cantidad;
                    }
                    c.num_transacciones++;

                    fseek(f, -sizeof(Cuenta), SEEK_CUR);


                    fwrite(&c, sizeof(Cuenta), 1, f);
                    break;
                }
            }
            fclose(f);
        }
        sem_post(sem_cuentas); //Barrera levantada

        if(cuenta_encontrada)
        {
            printf("[EXITO] Has depositado %.2f %s\n", cantidad, nom_divisa(divisa));


            struct msgbuf msj;
            msj.mtype = 1; //Tipo 1 = Mensaje para el Monitor

            msj.info.monitor.cuenta_origen = numero_cuenta;
            msj.info.monitor.tipo_op = tipo_op;
            msj.info.monitor.cantidad = cantidad;
            msj.info.monitor.divisa = divisa;


            //Enviamos el mensaje a la cola (msgid es la variable global)
            if(msgsnd(msgid, &msj, sizeof(msj.info), 0) == -1)
            {
                perror("Error enviando el mensaje al monitor");
            }
            else
            {
                printf("[SISTEMA] Notifiación enviada al Monitor.\n");
            }

        }
        else
        {
            printf("\n[INFO] Esta opción se programará más adelante, no te preocupes ;)\n");
        }

        sem_close(sem_cuentas);
        pthread_exit(NULL);
    }
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

int procesar_opcion(int opcion) {

    pthread_t thread;

    switch(opcion) {

        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        
            
            int *arg_opcion = malloc(sizeof(int));
            if(arg_opcion == NULL)
            {
                perror("Error reservando memoria para operacion");
                return 0;
            }
        
            *arg_opcion = opcion;

            if (pthread_create(&thread, NULL, ejecutar_operacion, arg_opcion) != 0)
            {
                perror("Error creando hilo de operación");
                free(arg_opcion);
                return 0;
            }
            pthread_join(thread, NULL);
            return 1;

        case 6:
            exit(0);
            printf("Cerrando sesión de la cuenta %d...\n", numero_cuenta);
            return 2;

        default:
            printf("Opcion invalida\n");
            return 0;
    }

    return 0;
}

int main(int argc, char *argv[]) {

    if(argc != 4)
    {
        fprintf(stderr, "Has hecho un uso incorrecto. Tiene que llamarse desde el banco\n");
        return 1;

    }

    numero_cuenta = atoi(argv[1]);
pipe_lectura = atoi(argv[2]);
msgid = atoi(argv[3]);

cargar_configuracion("../c/config.txt");

printf("\nSesión iniciada para la cuenta %d\n", numero_cuenta);

struct pollfd fds[2];
fds[0].fd = STDIN_FILENO;
fds[0].events = POLLIN;

fds[1].fd = pipe_lectura;
fds[1].events = POLLIN;

int opcion = 0;
int esperando_enter = 0;
fflush(stdout);

while (opcion != 6) {
    mostrar_menu();
    printf("Opción: ");
    scanf("%d", &opcion);
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF) {}

    procesar_opcion(opcion);

    int ret = poll(fds, 2, -1);

    if (ret > 0) {
        if (fds[1].revents & POLLIN) {
            char alerta[256];
            int bytes_leidos = read(pipe_lectura, alerta, sizeof(alerta) - 1);
            if (bytes_leidos > 0) {
                alerta[bytes_leidos] = '\0';
                printf("\nALERTA DEL BANCO %s\n", alerta);

                if(esperando_enter)
                {
                    printf("[INFO] Pulsa Enter para volver al menu...\n");
                }
                else
                {
                    printf("Opción: ");
                }

                mostrar_menu();
                printf("Opción: ");
                fflush(stdout);
            }
            else if(bytes_leidos == 0)
            {
                fds[1].fd = -1;
            }
        }

        if (fds[0].revents & POLLIN) {
            char buffer[10];
            if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
                if(esperando_enter)
                {
                    mostrar_menu();
                    printf("Opción: ");
                    fflush(stdout);
                    continue;;
                }

                opcion = atoi(buffer);

                if (opcion > 0 && opcion <= 6) {
                    int estado = procesar_opcion(opcion);

                    if(estado == 2)
                    {
                        opcion = 6;
                        continue;
                    }

                    if(estado == 1)
                    {
                        esperando_enter = 1;
                        printf("\n[INFO] Pulsa ENTER para volver al menú...\n");
                        fflush(stdout);
                        continue;
                    }
                }

                if (opcion != 6) {
                    mostrar_menu();
                    printf("Opción: ");
                    fflush(stdout);
                }
            }
        }
    }
}

close(pipe_lectura);
return 0;
}