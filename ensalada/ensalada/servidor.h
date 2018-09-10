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
    t_list* lista_clientes;
    t_log* logger;
}Servidor;

Servidor inicializar_servidor(t_log*, int);
void destruir_servidor(Servidor);
MensajeEntrante esperar_mensajes(Servidor);

#endif //SERVIDOR_H
