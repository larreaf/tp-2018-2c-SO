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
}Servidor;

int conectar_como_cliente(Servidor, char *, int, Proceso);
void cerrar_conexion(Servidor, int);
Servidor inicializar_servidor(t_log*, int, int*, Proceso);
void destruir_servidor(Servidor);
MensajeEntrante esperar_mensajes(Servidor);

#endif //SERVIDOR_H
