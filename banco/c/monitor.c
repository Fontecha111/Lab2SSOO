#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include "estructuras.h"

void analizar_transaccion(DatosMonitor *datos) {

    // TODO: implementar detección de anomalías

}

int main(int argc, char *argv[]) {

    if (argc < 2) {
        printf("Uso: monitor <msgid>\n");
        return 1;
    }

    int msgid = atoi(argv[1]);

    struct msgbuf mensaje;

    while (1) {

        msgrcv(msgid, &mensaje, sizeof(mensaje.info), 1, 0);

        analizar_transaccion(&mensaje.info.monitor);
    }

    return 0;
}
