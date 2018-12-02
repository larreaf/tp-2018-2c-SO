//
// Created by utnso on 26/09/18.
//

#ifndef SAFA_PCP_H
#define SAFA_PCP_H

#include <commons/collections/queue.h>
#include <commons/collections/list.h>
#include <ensalada/com.h>
#include <ensalada/mensaje.h>
#include <ensalada/protocolo.h>
#include <ensalada/servidor.h>
#include <ensalada/dtb.h>
#include "types.h"
#include <semaphore.h>
#include "plp.h"

typedef struct{
    t_queue* cola;
    int disponibilidad;
}Recurso;

typedef struct {
    t_queue* cola_ready;
    t_queue* cola_ready_aux;
    t_list* lista_block;
    t_list* lista_exec;
    pthread_mutex_t mutex_ready;
    pthread_mutex_t mutex_ready_aux;
    pthread_mutex_t mutex_block;
    pthread_mutex_t mutex_exec;
    pthread_mutex_t mutex_config;
    pthread_mutex_t mutex_pausa;
    sem_t semaforo_ready;
    sem_t semaforo_dummy;
    t_dictionary* recursos;
    int algoritmo_planificacion;
    int quantum;
    int retardo_planificacion;
    int finalizar_dtb;
    int cantidad_lineas_equipo_grande;
    t_log* logger;
}PCP;

CPU* seleccionar_cpu(t_list*);
void decrementar_procesos_asignados_cpu(ConexionesActivas, int);
PCP* inicializar_pcp(int, int, int, char*, int, int);
void destruir_recurso(void*);
void destruir_pcp(PCP*);
void destruir_dtb(void*);
DTB* crear_dtb(int, int);
void resetear_dtb_dummy(DTB*);
void desbloquear_dtb(PCP*, int);
void desbloquear_dtb_seleccionado(PCP*, DTB*);
void desbloquear_dtb_cargando_archivo(PCP*, int, char*, int, int);
void desbloquear_dtb_dummy(PCP*, int, char*);
void agregar_a_ready(PCP*, DTB*);
void agregar_a_ready_aux(PCP*, DTB*);
void agregar_a_block(PCP*, DTB*);
DTB* ready_a_exec(PCP*);
DTB* obtener_dtb_de_exec(PCP *pcp, int id);
DTB* conseguir_y_actualizar_dtb(PCP*, DTB*);
void imprimir_estado_pcp(PCP*);
DTB* encontrar_dtb_pcp(PCP*, int);
void* ejecutar_pcp(void*);
DTB* obtener_dtb_de_block(PCP*, int);

#endif //SAFA_PCP_H
