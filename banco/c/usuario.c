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

#define BALANCE_EPSILON 0.0001f

typedef struct {
    int tipo_op;
    int divisa;
    float cantidad;
    int cuenta_destino;
    int divisa_destino;
} OperacionDatos;

static const char *nom_divisa(int divisa)
{
    if(divisa == 1) return "EUR";
    if(divisa == 2) return "USD";
    if(divisa == 3) return "GBP";
    return "DESCONOCIDA";
}

static int leer_divisa_deposito(int *divisa)
{
    char buffer[128];
    char *endptr;
    long valor;

    printf("\nPor favor, selecciona la divisa que quieres usar: \n");
    printf("1. EUR\n");
    printf("2. USD\n");
    printf("3. GBP\n");
    printf("Divisa: ");
    fflush(stdout);

    if(fgets(buffer, sizeof(buffer), stdin) == NULL) return 0;

    errno = 0;
    valor = strtol(buffer, &endptr, 10);

    if(endptr == buffer) return 0;

    while(*endptr == ' ' || *endptr == '\t') endptr++;

    if(*endptr != '\n' && *endptr != '\0') return 0;

    if(errno == ERANGE || valor < 1 || valor > 3) return 0;

    *divisa = (int)valor;
    return 1;
}

static int leer_cantidad_deposito(float *cantidad, int divisa)
{
    char buffer[128];
    char *endptr;

    printf("\nPor favor, introduce la cantidad que quieres depositar (%s): ", nom_divisa(divisa));
    fflush(stdout);

    if(fgets(buffer, sizeof(buffer), stdin) == NULL) return 0;

    errno = 0;
    *cantidad = strtof(buffer, &endptr);

    if(endptr == buffer) return 0;

    while(*endptr == ' ' || *endptr == '\t') endptr++;

    if(*endptr != '\n' && *endptr != '\0') return 0;

    if(errno == ERANGE || !isfinite(*cantidad) || *cantidad <= 0.0f) return 0;

    return 1;
}

static int leer_cantidad_retiro(float *cantidad, int divisa)
{
    char buffer[128];
    char *endptr;

    printf("\nIntroduce la cantidad que quieres retirar (%s): ", nom_divisa(divisa));
    fflush(stdout);

    if(fgets(buffer, sizeof(buffer), stdin) == NULL) return 0;

    errno = 0;
    *cantidad = strtof(buffer, &endptr);

    if(endptr == buffer) return 0;

    while(*endptr == ' ' || *endptr == '\t') endptr++;

    if(*endptr != '\n' && *endptr != '\0') return 0;

    if(errno == ERANGE || !isfinite(*cantidad) || *cantidad <= 0.0f) return 0;

    return 1;
}

static int leer_cuenta_destino(int *cuenta_destino)
{
    char buffer[128];
    char *endptr;
    long valor;

    printf("\nIntroduce el número de cuenta de destino: ");
    fflush(stdout);

    if(fgets(buffer, sizeof(buffer), stdin) == NULL) return 0;

    errno = 0;
    valor = strtol(buffer, &endptr, 10);

    if(endptr == buffer) return 0;

    while(*endptr == ' ' || *endptr == '\t') endptr++;

    if(*endptr != '\n' && *endptr != '\0') return 0;

    if(errno == ERANGE || valor <= 0) return 0;

    *cuenta_destino = (int)valor;
    return 1;
}

static int leer_cantidad_transferencia(float *cantidad, int divisa)
{
    char buffer[128];
    char *endptr;

    printf("\nIntroduce la cantidad que quieres transferir (%s): ", nom_divisa(divisa));
    fflush(stdout);

    if(fgets(buffer, sizeof(buffer), stdin) == NULL) return 0;

    errno = 0;
    *cantidad = strtof(buffer, &endptr);

    if(endptr == buffer) return 0;

    while(*endptr == ' ' || *endptr == '\t') endptr++;

    if(*endptr != '\n' && *endptr != '\0') return 0;

    if(errno == ERANGE || !isfinite(*cantidad) || *cantidad <= 0.0f) return 0;

    return 1;
}

static float obtener_tasa_cambio(int divisa_origen, int divisa_destino)
{
    if(divisa_origen == divisa_destino) return 1.0f;

    if(divisa_origen == 1) {
        if(divisa_destino == 2) return config_banco.cambio_usd;
        if(divisa_destino == 3) return config_banco.cambio_gbp;
    }
    if(divisa_origen == 2) {
        if(divisa_destino == 1) return 1.0f / config_banco.cambio_usd;
        if(divisa_destino == 3) return config_banco.cambio_gbp / config_banco.cambio_usd;
    }
    if(divisa_origen == 3) {
        if(divisa_destino == 1) return 1.0f / config_banco.cambio_gbp;
        if(divisa_destino == 2) return config_banco.cambio_usd / config_banco.cambio_gbp;
    }

    return 0.0f;
}

static int leer_cantidad_cambio(float *cantidad, int divisa)
{
    char buffer[128];
    char *endptr;

    printf("Cantidad que quieres cambiar: ");
    fflush(stdout);

    if(fgets(buffer, sizeof(buffer), stdin) == NULL) return 0;

    errno = 0;
    *cantidad = strtof(buffer, &endptr);

    if(endptr == buffer) return 0;

    while(*endptr == ' ' || *endptr == '\t') endptr++;

    if(*endptr != '\n' && *endptr != '\0') return 0;

    if(errno == ERANGE || !isfinite(*cantidad) || *cantidad <= 0.0f) return 0;

    return 1;
}

static int leer_divisa_destino(int *divisa_destino)
{
    char buffer[128];
    char *endptr;
    long valor;

    printf("A que divisa quieres cambiar: \n");
    printf("1. EUR\n");
    printf("2. USD\n");
    printf("3. GBP\n");
    printf("Divisa de destino: ");
    fflush(stdout);

    if(fgets(buffer, sizeof(buffer), stdin) == NULL) return 0;

    errno = 0;
    valor = strtol(buffer, &endptr, 10);

    if(endptr == buffer) return 0;

    while(*endptr == ' ' || *endptr == '\t') endptr++;

    if(*endptr != '\n' && *endptr != '\0') return 0;

    if(errno == ERANGE || valor < 1 || valor > 3) return 0;

    *divisa_destino = (int)valor;
    return 1;
}

static sem_t *abrir_semaforo_cuentas(void)
{
    sem_t *sem = sem_open("/sem_cuentas", 0);
    if(sem == SEM_FAILED)
    {
        perror("Error abriendo semáforo de cuentas");
    }
    return sem;
}

static void cerrar_semaforo(sem_t *sem)
{
    if(sem != NULL && sem != SEM_FAILED)
    {
        sem_close(sem);
    }
}

static void enviar_notificacion_monitor(OperacionDatos *op_datos)
{
    struct msgbuf msj;
    msj.mtype = 1;
    msj.info.monitor.cuenta_origen = numero_cuenta;
    msj.info.monitor.tipo_op = op_datos->tipo_op;
    msj.info.monitor.cantidad = op_datos->cantidad;
    msj.info.monitor.divisa = op_datos->divisa;
    msj.info.monitor.cuenta_destino = op_datos->cuenta_destino;

    if(msgsnd(msgid, &msj, sizeof(msj.info), 0) == -1)
    {
        perror("Error enviando el mensaje al monitor");
    }
    else
    {
        printf("[SISTEMA] Notificación enviada al Monitor.\n");
    }
}

static void operacion_consultar_saldo(sem_t *sem_cuentas)
{
    Cuenta c;
    sem_wait(sem_cuentas);

    FILE *f = fopen("cuentas.dat", "rb");
    if(f != NULL)
    {
        while(fread(&c, sizeof(Cuenta), 1, f))
        {
            if(c.numero_cuenta == numero_cuenta)
            {
                printf("\n[SALDO ACTUAL] EUR: %.2f | USD: %.2f | GBP: %.2f\n",
                       c.saldo_eur, c.saldo_usd, c.saldo_gbp);
                break;
            }
        }
        fclose(f);
    }
    sem_post(sem_cuentas);
}

static void operacion_deposito(sem_t *sem_cuentas, OperacionDatos *op_datos)
{
    if(!leer_divisa_deposito(&op_datos->divisa))
    {
        printf("[ERROR] La divisa introducida no es válida. Elige 1 (EUR), 2 (USD) o 3 (GBP)\n");
        return;
    }

    if(!leer_cantidad_deposito(&op_datos->cantidad, op_datos->divisa))
    {
        printf("[ERROR] La cantidad introducida no es válida. Introduce un número positivo y válido\n");
        return;
    }

    Cuenta c;
    int cuenta_encontrada = 0;

    sem_wait(sem_cuentas);
    FILE *f = fopen("cuentas.dat", "r+b");
    if(f != NULL)
    {
        while(fread(&c, sizeof(Cuenta), 1, f))
        {
            if(c.numero_cuenta == numero_cuenta)
            {
                cuenta_encontrada = 1;
                switch(op_datos->divisa)
                {
                    case 1: c.saldo_eur += op_datos->cantidad; break;
                    case 2: c.saldo_usd += op_datos->cantidad; break;
                    case 3: c.saldo_gbp += op_datos->cantidad; break;
                }
                c.num_transacciones++;
                fseek(f, -sizeof(Cuenta), SEEK_CUR);
                fwrite(&c, sizeof(Cuenta), 1, f);
                break;
            }
        }
        fclose(f);
    }
    sem_post(sem_cuentas);

    if(cuenta_encontrada)
    {
        printf("[EXITO] Has depositado %.2f %s\n", op_datos->cantidad, nom_divisa(op_datos->divisa));
        enviar_notificacion_monitor(op_datos);
    }
}

static void operacion_retiro(sem_t *sem_cuentas, OperacionDatos *op_datos)
{
    if(!leer_divisa_deposito(&op_datos->divisa))
    {
        printf("[ERROR] La divisa no es válida. Elige 1 (EUR), 2 (USD) o 3 (GBP)\n");
        return;
    }

    if(!leer_cantidad_retiro(&op_datos->cantidad, op_datos->divisa))
    {
        printf("[ERROR] La cantidad no es válida. Introduce un número positivo y válido\n");
        return;
    }

    Cuenta c;
    int cuenta_encontrada = 0;
    int retiro_realizado = 0;
    float saldo_disponible = 0.0f;

    sem_wait(sem_cuentas);
    FILE *f = fopen("cuentas.dat", "r+b");
    if(f != NULL)
    {
        while(fread(&c, sizeof(Cuenta), 1, f))
        {
            if(c.numero_cuenta == numero_cuenta)
            {
                cuenta_encontrada = 1;
                float saldo_actual = 0.0f;
                switch(op_datos->divisa)
                {
                    case 1: saldo_actual = c.saldo_eur; break;
                    case 2: saldo_actual = c.saldo_usd; break;
                    case 3: saldo_actual = c.saldo_gbp; break;
                }
                saldo_disponible = saldo_actual;

                if((saldo_actual + BALANCE_EPSILON) >= op_datos->cantidad)
                {
                    switch(op_datos->divisa)
                    {
                        case 1: c.saldo_eur -= op_datos->cantidad; break;
                        case 2: c.saldo_usd -= op_datos->cantidad; break;
                        case 3: c.saldo_gbp -= op_datos->cantidad; break;
                    }
                    retiro_realizado = 1;
                    c.num_transacciones++;
                    fseek(f, -sizeof(Cuenta), SEEK_CUR);
                    fwrite(&c, sizeof(Cuenta), 1, f);
                }
                break;
            }
        }
        fclose(f);
    }
    sem_post(sem_cuentas);

    if(!cuenta_encontrada)
    {
        printf("[ERROR] No se encontro la cuenta %d\n", numero_cuenta);
    }
    else if(!retiro_realizado)
    {
        printf("[ERROR] Saldo insuficiente para retirar %.2f %s. Saldo disponible: %.2f %s\n",
               op_datos->cantidad, nom_divisa(op_datos->divisa),
               saldo_disponible, nom_divisa(op_datos->divisa));
    }
    else
    {
        printf("[EXITO] Has retirado %.2f %s\n", op_datos->cantidad, nom_divisa(op_datos->divisa));
        enviar_notificacion_monitor(op_datos);
    }
}

static void operacion_transferencia(sem_t *sem_cuentas, OperacionDatos *op_datos)
{
    if(!leer_cuenta_destino(&op_datos->cuenta_destino))
    {
        printf("[ERROR] Número de cuenta de destino no válido\n");
        return;
    }

    if(op_datos->cuenta_destino == numero_cuenta)
    {
        printf("[ERROR] No puedes transferir dinero a tu propia cuenta\n");
        return;
    }

    if(!leer_divisa_deposito(&op_datos->divisa))
    {
        printf("[ERROR] Esa divisa no es válida. Elige 1 (EUR), 2 (USD) o 3 (GBP)\n");
        return;
    }

    if(!leer_cantidad_transferencia(&op_datos->cantidad, op_datos->divisa))
    {
        printf("[ERROR] Cantidad no válida. Introduce un número positivo y válido\n");
        return;
    }

    Cuenta cuenta_origen, cuenta_dest;
    int origen_encontrado = 0, destino_encontrado = 0, transferencia_realizada = 0;
    long pos_origen = -1, pos_destino = -1;
    float saldo_disponible = 0.0f;

    sem_wait(sem_cuentas);
    FILE *f = fopen("cuentas.dat", "r+b");
    if(f != NULL)
    {
        Cuenta c;
        while(fread(&c, sizeof(Cuenta), 1, f))
        {
            long pos_actual = ftell(f) - (long)sizeof(Cuenta);

            if(c.numero_cuenta == numero_cuenta && !origen_encontrado)
            {
                cuenta_origen = c;
                pos_origen = pos_actual;
                origen_encontrado = 1;
            }

            if(c.numero_cuenta == op_datos->cuenta_destino && !destino_encontrado)
            {
                cuenta_dest = c;
                pos_destino = pos_actual;
                destino_encontrado = 1;
            }

            if(origen_encontrado && destino_encontrado) break;
        }

        if(origen_encontrado && destino_encontrado)
        {
            float saldo_origen = 0.0f;
            switch(op_datos->divisa)
            {
                case 1: saldo_origen = cuenta_origen.saldo_eur; break;
                case 2: saldo_origen = cuenta_origen.saldo_usd; break;
                case 3: saldo_origen = cuenta_origen.saldo_gbp; break;
            }
            saldo_disponible = saldo_origen;

            if((saldo_origen + BALANCE_EPSILON) >= op_datos->cantidad)
            {
                switch(op_datos->divisa)
                {
                    case 1:
                        cuenta_origen.saldo_eur -= op_datos->cantidad;
                        cuenta_dest.saldo_eur   += op_datos->cantidad;
                        break;
                    case 2:
                        cuenta_origen.saldo_usd -= op_datos->cantidad;
                        cuenta_dest.saldo_usd   += op_datos->cantidad;
                        break;
                    case 3:
                        cuenta_origen.saldo_gbp -= op_datos->cantidad;
                        cuenta_dest.saldo_gbp   += op_datos->cantidad;
                        break;
                }
                transferencia_realizada = 1;
                cuenta_origen.num_transacciones++;
                cuenta_dest.num_transacciones++;

                fseek(f, pos_origen, SEEK_SET);
                fwrite(&cuenta_origen, sizeof(Cuenta), 1, f);

                fseek(f, pos_destino, SEEK_SET);
                fwrite(&cuenta_dest, sizeof(Cuenta), 1, f);
            }
        }
        fclose(f);
    }
    sem_post(sem_cuentas);

    if(!origen_encontrado)
    {
        printf("[ERROR] No se encontro la cuenta %d\n", numero_cuenta);
    }
    else if(!destino_encontrado)
    {
        printf("[ERROR] La cuenta de destino %d no existe\n", op_datos->cuenta_destino);
    }
    else if(!transferencia_realizada)
    {
        printf("[ERROR] Saldo insuficiente para transferir %.2f %s. Saldo disponible: %.2f %s\n",
               op_datos->cantidad, nom_divisa(op_datos->divisa),
               saldo_disponible, nom_divisa(op_datos->divisa));
    }
    else
    {
        printf("[EXITO] Se ha completado la transferencia de %.2f %s a la cuenta %d\n",
               op_datos->cantidad, nom_divisa(op_datos->divisa), op_datos->cuenta_destino);
        enviar_notificacion_monitor(op_datos);
    }
}

static void operacion_mover_divisas(sem_t *sem_cuentas, OperacionDatos *op_datos)
{
    int divisa_origen, divisa_destino;
    float saldo_actual = 0.0f;

    printf("\nPor favor, selecciona la divisa que quieres cambiar: \n");
    printf("1. EUR\n");
    printf("2. USD\n");
    printf("3. GBP\n");
    printf("Divisa origen: ");
    fflush(stdout);

    if(!leer_divisa_deposito(&divisa_origen))
    {
        printf("[ERROR] La divisa de origen no es válida\n");
        return;
    }

    Cuenta c;
    int cuenta_encontrada = 0;

    sem_wait(sem_cuentas);
    FILE *f = fopen("cuentas.dat", "rb");
    if(f != NULL)
    {
        while(fread(&c, sizeof(Cuenta), 1, f))
        {
            if(c.numero_cuenta == numero_cuenta)
            {
                cuenta_encontrada = 1;
                switch(divisa_origen)
                {
                    case 1: saldo_actual = c.saldo_eur; break;
                    case 2: saldo_actual = c.saldo_usd; break;
                    case 3: saldo_actual = c.saldo_gbp; break;
                }
                break;
            }
        }
        fclose(f);
    }
    sem_post(sem_cuentas);

    if(!cuenta_encontrada)
    {
        printf("[ERROR] La cuenta no ha sido encontrada\n");
        return;
    }

    printf("\n[SALDO ACTUAL] %.2f %s\n", saldo_actual, nom_divisa(divisa_origen));

    if(!leer_cantidad_cambio(&op_datos->cantidad, divisa_origen))
    {
        printf("[ERROR] La cantidad no es válida\n");
        return;
    }

    if(op_datos->cantidad > saldo_actual)
    {
        printf("[ERROR] El saldo no es suficiente para cambiar %.2f %s\n",
               op_datos->cantidad, nom_divisa(divisa_origen));
        return;
    }

    if(!leer_divisa_destino(&divisa_destino))
    {
        printf("[ERROR] La divisa de destino no es válida\n");
        return;
    }

    if(divisa_origen == divisa_destino)
    {
        printf("[ERROR] La divisa de destino no puede ser la misma\n");
        return;
    }

    float tasa_cambio = obtener_tasa_cambio(divisa_origen, divisa_destino);
    float cantidad_convertida = op_datos->cantidad * tasa_cambio;

    sem_wait(sem_cuentas);
    f = fopen("cuentas.dat", "r+b");
    if(f != NULL)
    {
        while(fread(&c, sizeof(Cuenta), 1, f))
        {
            if(c.numero_cuenta == numero_cuenta)
            {
                switch(divisa_origen)
                {
                    case 1: c.saldo_eur -= op_datos->cantidad; break;
                    case 2: c.saldo_usd -= op_datos->cantidad; break;
                    case 3: c.saldo_gbp -= op_datos->cantidad; break;
                }
                switch(divisa_destino)
                {
                    case 1: c.saldo_eur += cantidad_convertida; break;
                    case 2: c.saldo_usd += cantidad_convertida; break;
                    case 3: c.saldo_gbp += cantidad_convertida; break;
                }
                c.num_transacciones++;
                fseek(f, -sizeof(Cuenta), SEEK_CUR);
                fwrite(&c, sizeof(Cuenta), 1, f);
                break;
            }
        }
        fclose(f);
    }
    sem_post(sem_cuentas);

    printf("[CORRECTO] %.2f %s se han convertido a %.2f %s (tasa: %.4f)\n",
           op_datos->cantidad, nom_divisa(divisa_origen),
           cantidad_convertida, nom_divisa(divisa_destino), tasa_cambio);

    op_datos->divisa = divisa_origen;
    op_datos->cuenta_destino = divisa_destino;
    enviar_notificacion_monitor(op_datos);
}

void *ejecutar_operacion(void *arg)
{
    int tipo_op = *(int*)arg;
    free(arg);

    sem_t *sem_cuentas = abrir_semaforo_cuentas();
    if(sem_cuentas == SEM_FAILED)
    {
        pthread_exit(NULL);
    }

    OperacionDatos op_datos = {0};
    op_datos.tipo_op = tipo_op;

    switch(tipo_op)
    {
        case 1:
            operacion_deposito(sem_cuentas, &op_datos);
            break;
        case 2:
            operacion_retiro(sem_cuentas, &op_datos);
            break;
        case 3:
            operacion_transferencia(sem_cuentas, &op_datos);
            break;
        case 4:
            operacion_consultar_saldo(sem_cuentas);
            break;
        case 5:
            operacion_mover_divisas(sem_cuentas, &op_datos);
            break;
        default:
            printf("\n[INFO] Esto se programará más adelante, tranquilo Titel ;)\n");
            break;
    }

    cerrar_semaforo(sem_cuentas);
    pthread_exit(NULL);
}

void mostrar_menu()
{
    printf("\n--- MENU USUARIO ---\n");
    printf("1. Deposito\n");
    printf("2. Retiro\n");
    printf("3. Transferencia\n");
    printf("4. Consultar saldo\n");
    printf("5. Mover divisas\n");
    printf("6. Salir\n");
}

static int lanzar_operacion_en_hilo(int tipo_op)
{
    pthread_t thread;
    int *arg_opcion = malloc(sizeof(int));
    if(arg_opcion == NULL)
    {
        perror("Error reservando memoria para operacion");
        return 0;
    }
    *arg_opcion = tipo_op;

    if(pthread_create(&thread, NULL, ejecutar_operacion, arg_opcion) != 0)
    {
        perror("Error creando hilo de operación");
        free(arg_opcion);
        return 0;
    }
    pthread_join(thread, NULL);
    return 1;
}

int procesar_opcion(int opcion)
{
    switch(opcion)
    {
        case 1:
            return lanzar_operacion_en_hilo(1);
        case 2:
            return lanzar_operacion_en_hilo(2);
        case 3:
            return lanzar_operacion_en_hilo(3);
        case 4:
            return lanzar_operacion_en_hilo(4);
        case 5:
            return lanzar_operacion_en_hilo(5);
        case 6:
            printf("Cerrando sesión de la cuenta %d...\n", numero_cuenta);
            return 2;
        default:
            printf("Opcion invalida\n");
            return 0;
    }
}

int main(int argc, char *argv[])
{
    if(argc != 4)
    {
        fprintf(stderr, "Has hecho un uso incorrecto. Tiene que llamarse desde el banco\n");
        return 1;
    }

    numero_cuenta = atoi(argv[1]);
    pipe_lectura  = atoi(argv[2]);
    msgid         = atoi(argv[3]);

    cargar_configuracion("../c/config.txt");

    printf("\nSesión iniciada para la cuenta %d\n", numero_cuenta);

    struct pollfd fds[2];
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;
    fds[1].fd = pipe_lectura;
    fds[1].events = POLLIN;

    int opcion = 0;
    int esperando_enter = 0;

    mostrar_menu();
    printf("Opción: ");
    fflush(stdout);

    while(opcion != 6)
    {
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

                    if(esperando_enter)
                        printf("[INFO] Pulsa Enter para volver al menu...\n");
                    else
                        printf("Opción: ");

                    fflush(stdout);
                }
                else if(bytes_leidos == 0)
                {
                    fds[1].fd = -1;
                }
            }

            if(fds[0].revents & POLLIN)
            {
                char buffer[10];
                if(fgets(buffer, sizeof(buffer), stdin) != NULL)
                {
                    if(esperando_enter)
                    {
                        esperando_enter = 0;
                        mostrar_menu();
                        printf("Opción: ");
                        fflush(stdout);
                        continue;
                    }

                    opcion = atoi(buffer);

                    if(opcion > 0 && opcion <= 6)
                    {
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

    close(pipe_lectura);
    return 0;
}