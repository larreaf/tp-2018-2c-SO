//
// Created by utnso on 26/09/18.
//

#ifndef SAFA_PLP_H
#define SAFA_PLP_H

#include <commons/collections/list.h>
#include "types.h"
#include "pcp.h"
#include <semaphore.h>
#include <errno.h>

typedef struct {
    t_list* lista_new;
    t_queue* cola_ready;
    pthread_mutex_t mutex_new;
    pthread_mutex_t mutex_ready;
    sem_t semaforo_new;
    sem_t semaforo_multiprogramacion;
    t_log* logger;
    t_list* metricas_dtbs;
    pthread_mutex_t mutex_metricas;
    pthread_mutex_t mutex_pausa;
}PLP;

PLP* inicializar_plp(int, char*, int);
void destruir_plp(PLP*);
void actualizar_cantidad_instrucciones_en_new(PLP*, int);
void agregar_a_new(PLP*, DTB*);
void pasar_new_a_ready(PLP*, int);
void imprimir_estado_plp(PLP*);
DTB* obtener_dtb_de_new(PLP*, int, bool);
char* codigo_error_a_string(int);
void abortar_dtb(PLP*, int, int, t_dictionary*);
void abortar_dtb_seleccionado(PLP*, DTB*, int, t_dictionary*);
void* ejecutar_plp(void*);

#endif //SAFA_PLP_H
