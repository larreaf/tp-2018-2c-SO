#ifndef SERVIDOR_H
#define SERVIDOR_H

#include <sys/select.h>
#include <semaphore.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include "com.h"
#include "protocolo.h"
#include "mensaje.h"

typedef struct{
    int socket;
    int id;
    int cantidad_procesos_asignados;
}CPU;

typedef struct{
    int socket;
    Proceso t_proceso;
}ConexionCliente;

typedef struct{
    int inicializado;
    int socket;
    int* procesos_conectados;
    int* procesos_permitidos;
    int socket_eldiego;
    int socket_safa;
    int socket_mdj;
    int socket_fm9;
    t_list* lista_clientes;
    t_list* lista_cpus;
    t_log* logger;
    Proceso t_proceso_host;
    sem_t semaforo_safa;

}ConexionesActivas;

int conectar_como_cliente(ConexionesActivas, char *, int, Proceso);
void cerrar_conexion(ConexionesActivas, int);
ConexionesActivas inicializar_conexiones_activas(t_log *, char* ip, int puerto, int *, Proceso);
void destruir_conexiones_activas(ConexionesActivas);
MensajeDinamico* esperar_mensajes(ConexionesActivas);

#endif //SERVIDOR_H
