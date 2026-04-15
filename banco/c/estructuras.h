#ifndef ESTRUCTURAS_H
#define ESTRUCTURAS_H

#include <sys/types.h>

#define ID_INICIAL 1001

typedef struct {
    int numero_cuenta;
    char titular[50];
    float saldo_eur;
    float saldo_usd;
    float saldo_gbp;
    int num_transacciones;
} Cuenta;

typedef struct {
    int cuenta_origen;
    int cuenta_destino;
    int tipo_op;
    float cantidad;
    int divisa;
    char timestamp[20];
} DatosMonitor;

typedef struct {
    int cuenta_id;
    pid_t pid_hijo;
    int tipo_op;
    float cantidad;
    int divisa;
    int estado;
    char timestamp[20];
} DatosLog;

struct msgbuf {
    long mtype;
    union {
        DatosMonitor monitor;
        DatosLog log;
    } info;
};

typedef struct {
    int proximo_id;

    float lim_retiro_eur;
    float lim_retiro_usd;
    float lim_retiro_gbp;

    float lim_transf_eur;
    float lim_transf_usd;
    float lim_transf_gbp;

    int umbral_retiros;
    int umbral_transferencias;

    int num_hilos;
    char archivo_cuentas[100];
    char archivo_log[100];

    float cambio_usd;
    float cambio_gbp;

}Configuracion;

#endif
