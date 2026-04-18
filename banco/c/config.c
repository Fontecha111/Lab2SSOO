#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"

Configuracion config_banco;

void cargar_configuracion(const char *nom_archivo)
{
    FILE *archivo = fopen(nom_archivo, "r");
    if(archivo == NULL)
    {
        perror("Error. No se ha podido abrir el archivo de configuración.");
        exit(1);
    }

    char linea[256];
    char clave[100];
    char valor_str[100];

    while(fgets(linea, sizeof(linea), archivo) != NULL)
    {
        if(linea[0] == '\n' || linea[0] == '#' || linea[0] == '\r')
        {
            continue;
        }
    

        if(sscanf(linea, "%[^=]=%s", clave, valor_str) == 2)
        {
            if(strcmp(clave, "PROXIMO_ID") == 0)
            {
                config_banco.proximo_id = atoi(valor_str);
            } 
            else if(strcmp(clave, "LIM_DEP_EUR") == 0)
            {
                config_banco.lim_dep_eur = atof(valor_str);
            }
            else if(strcmp(clave, "LIM_DEP_USD") == 0)
            {
                config_banco.lim_dep_usd = atof(valor_str);
            }
            else if(strcmp(clave, "LIM_DEP_GBP") == 0)
            {
                config_banco.lim_dep_gbp = atof(valor_str);
            }
            else if (strcmp(clave, "LIM_RETIRO_EUR") == 0)
            {
                config_banco.lim_retiro_eur = atof(valor_str);
            }   
            else if (strcmp(clave, "LIM_RETIRO_USD") == 0)
            {
                config_banco.lim_retiro_usd = atof(valor_str);
            }
            else if (strcmp(clave, "LIM_RETIRO_GBP") == 0)
            {
                config_banco.lim_retiro_gbp = atof(valor_str);
            }
            else if (strcmp(clave, "LIM_TRANSF_EUR") == 0)
            {
                config_banco.lim_transf_eur = atof(valor_str);
            }
            else if (strcmp(clave, "LIM_TRANSF_USD") == 0)
            {
                config_banco.lim_transf_usd = atof(valor_str);
            }
            else if (strcmp(clave, "LIM_TRANSF_GBP") == 0)
            {
                config_banco.lim_transf_gbp = atof(valor_str);
            }
            else if (strcmp(clave, "UMBRAL_RETIROS") == 0)
            {
                config_banco.umbral_retiros = atoi(valor_str);
            }
            else if (strcmp(clave, "UMBRAL_TRANSFERENCIAS") == 0)
            {
                config_banco.umbral_transferencias = atoi(valor_str);
            }
            else if (strcmp(clave, "NUM_HILOS") == 0)
            {
                config_banco.num_hilos = atoi(valor_str);
            }
            else if (strcmp(clave, "ARCHIVO_CUENTAS") == 0)
            {
                strcpy(config_banco.archivo_cuentas, valor_str);
            }
            else if (strcmp(clave, "ARCHIVO_LOG") == 0)
            {
                strcpy(config_banco.archivo_log, valor_str);
            }   
            else if (strcmp(clave, "CAMBIO_USD") == 0)
            {
                config_banco.cambio_usd = atof(valor_str);
            }
            else if ((strcmp(clave, "CAMBIO_GBP") == 0))
            {
                config_banco.cambio_gbp = atof(valor_str);
            }
            
        }
    }

    fclose(archivo);

    
}