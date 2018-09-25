#ifndef SERVIDOR_H
#define SERVIDOR_H

#include <sys/select.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include "protocolo.h"
#include "com.h"

typedef struct{
    int socket;
    Proceso t_proceso;
}ConexionCliente;

typedef struct{
    int socket;
    int header;
    Proceso t_proceso;
}MensajeEntrante;

typedef struct{
    int inicializado;
    int socket;
    int* procesos_conectados;
    int* procesos_permitidos;
    t_list* lista_clientes;
    t_log* logger;
    Proceso t_proceso_host;
}ConexionesActivas;

int conectar_como_cliente(ConexionesActivas, char *, int, Proceso);
void cerrar_conexion(ConexionesActivas, int);
ConexionesActivas inicializar_conexiones_activas(t_log *, int, int *, Proceso);
void destruir_conexiones_activas(ConexionesActivas);
MensajeEntrante esperar_mensajes(ConexionesActivas);

#endif //SERVIDOR_H
