#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include "estructuras.h"
#include "config.h"

void analizar_transaccion(DatosMonitor *datos) {

    // TODO: implementar detección de anomalías
    printf("\n[MONITOR] Interceptada operación de la cuenta: %d\n", datos->cuenta_origen);

    //Si la operación es un depósito
    if(datos->tipo_op == 1)
    {
        printf("[MONITOR] Tipo: DEPOSITO | Cantidad %.2f\n", datos->cantidad);


        //Comprobamos si se supera algún límite absurdo para probar
        //Luego se usarán los límites reales de config_banco
        if(datos->cantidad > 3000.0)
        {
            printf("[ALERTA MONITOR] ¡Se ha detectado un movimiento inusual!\n");

        }
        else
        {
            printf("[MONITOR] Transacción normal. No ha saltado ninguna alerta\n");
        }
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

        msgrcv(msgid, &mensaje, sizeof(mensaje.info), 1, 0);

        analizar_transaccion(&mensaje.info.monitor);
    }

    return 0;
}
