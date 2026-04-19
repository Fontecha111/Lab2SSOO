#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <time.h>
#include <string.h>
#include "estructuras.h"
#include "config.h"

static const char *nom_divisa(int divisa)
{
    if(divisa == 1)
    {
        return "EUR";
    }
    else if(divisa == 2)
    {
        return "USD";
    }
    else if(divisa == 3)
    {
        return "GBP";
    }
    else
    {
        return "DESCONOCIDA";
    }
}

static const char *nombre_operacion(int tipo_op)
{
    if(tipo_op == 1)
    {
        return "DEPÓSITO";
    }
    if(tipo_op == 2)
    {
        return "RETIRO";
    }
    if(tipo_op == 3)
    {
        return "TRANSFERENCIA";
    }
    if(tipo_op == 4)
    {
        return "CONSULTA_SALDO";
    }
    if(tipo_op == 5)
    {
        return "MOVER_DIVISAS";
    }

    return "DESCONOCIDA";
}

static float obtener_limite_operacion(int tipo_op, int divisa)
{
    if(tipo_op == 1)
    {
        if(divisa == 1)
        {
            return config_banco.lim_dep_eur;
        }
        if(divisa == 2)
        {
            return config_banco.lim_dep_usd;
        }
        if(divisa == 3)
        {
            return config_banco.lim_dep_gbp;
        }
    }

    if(tipo_op == 2)
    {
        if(divisa == 1)
        {
            return config_banco.lim_dep_eur;
        }
        if(divisa == 2)
        {
            return config_banco.lim_dep_usd;
        }
        if(divisa == 3)
        {
            return config_banco.lim_dep_gbp;
        }
    }

    if(tipo_op == 3)
    {
        if(divisa == 1)
        {
            return config_banco.lim_transf_eur;
        }
        if(divisa == 2)
        {
            return config_banco.lim_transf_usd;
        }
        if(divisa == 3)
        {
            return config_banco.lim_transf_gbp;
        }
    }

    return 0.0f;
}

void analizar_transaccion(DatosMonitor *datos, int msgid) {

    // TODO: implementar detección de anomalías
    printf("\n[MONITOR] Interceptada operación de la cuenta: %d\n", datos->cuenta_origen);

    FILE *log = fopen(config_banco.archivo_log, "a"); //"a" de append (añadir al final)
    if(log != NULL)
    {
        //Obtenemos la hora actual para que quede bonito
        time_t t = time(NULL);
        struct tm *tm = localtime(&t);

        if(datos->tipo_op == 3)
        {
            fprintf(log, "[%02d:%02d:%02d] ORIGEN: %d | DESTINO: %d | OP: %s(%d) | CANT: %.2f | DIVISA: %d (%s)\n", tm->tm_hour, tm->tm_min, tm->tm_sec, datos->cuenta_origen, datos->cuenta_destino, nombre_operacion(datos->tipo_op), datos->tipo_op, datos->cantidad, datos->divisa, nom_divisa(datos->divisa));
        }
        else if(datos->tipo_op == 5)
        {
            fprintf(log, "[%02d:%02d:%02d] CUENTA: %d | OP: %s(%d) | CANTIDAD: %.2f | De: (%s) | A: (%s)\n", tm->tm_hour, tm->tm_min, tm->tm_sec, datos->cuenta_origen, nombre_operacion(datos->tipo_op), datos->tipo_op, datos->cantidad, nom_divisa(datos->divisa), nom_divisa(datos->cuenta_destino));
        }
        else
        {
            fprintf(log, "[%02d:%02d:%02d] CUENTA: %d | OP: %s(%d) | CANT: %.2f | DIVISA: %d (%s)\n", tm->tm_hour, tm->tm_min, tm->tm_sec, datos->cuenta_origen, nombre_operacion(datos->tipo_op), datos->tipo_op, datos->cantidad, datos->divisa, nom_divisa(datos->divisa));
        }

        fclose(log);
    }
    else
    {
        perror("Error escribiendo en el log");
    }
        

    //Si la operación es un depósito
    if(datos->tipo_op == 1 || datos->tipo_op == 2 || datos->tipo_op == 3)
    {
        float limite = obtener_limite_operacion(datos->tipo_op, datos->divisa);

        printf("[MONITOR] Tipo: %s | Cantidad: %.2f %s\n", nombre_operacion(datos->tipo_op), datos->cantidad, nom_divisa(datos->divisa));


        //Comprobamos si se supera algún límite absurdo para probar
        //Luego se usarán los límites reales de config_banco
        if(limite > 0.0f && datos->cantidad > limite)
        {
            printf("[ALERTA MONITOR] ¡Se ha detectado un movimiento inusual! Avisando al banco...\n");


            //Enviamos un mensaje de ALERTA (Tipo 2) a la cola
            struct msgbuf msj_alerta;
            msj_alerta.mtype = 2;
            msj_alerta.info.monitor = *datos; //Copiamos los datos


            msgsnd(msgid, &msj_alerta, sizeof(msj_alerta.info), 0);
        }
        else
        {
            printf("[MONITOR] Transacción normal. No ha saltado ninguna alerta\n");
        }
    }
    else if(datos->tipo_op == 5)
    {
        printf("[MONITOR] Tipo: %s | Cambio: %.2f %s -> %s\n", nombre_operacion(datos->tipo_op), datos->cantidad, nom_divisa(datos->divisa), nom_divisa(datos->cuenta_destino));
        printf("[MONITOR] La transacción no ha dejado alertas\n");
    }

}

int main(int argc, char *argv[]) {

    if (argc < 2) {
        printf("Uso: monitor <msgid>\n");
        return 1;
    }

    cargar_configuracion("../c/config.txt");
    printf("[MONITOR] Listo y vigilando la cola de mensajes...\n");

    int msgid = atoi(argv[1]);

    struct msgbuf mensaje;

    while (1) {

        if(msgrcv(msgid, &mensaje, sizeof(mensaje.info), 1, 0) != -1)
        {
            analizar_transaccion(&mensaje.info.monitor, msgid);
        }
    }

    return 0;
}
