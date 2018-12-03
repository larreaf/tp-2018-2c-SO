#ifndef DTB_H
#define DTB_H

#include <commons/collections/list.h>
#include <time.h>
#include "mensaje.h"

typedef enum {
    BLOQUEAR,
    READY,
    DTB_EXIT,
}t_accion_post_instruccion;

typedef struct{
    char* path;
    int direccion_memoria;
    int equipo_grande;
}ArchivoAbierto;

typedef struct{
    int id;
    char* path_script;
    int program_counter;
    int inicializado;
    int quantum;
    int cargando;
    t_accion_post_instruccion status;
    t_list* archivos_abiertos;
    time_t ready_ts;
}DTB;

typedef struct{
    int id_dtb;
    int cantidad_instrucciones_ejecutadas;
    int cantidad_instrucciones_dma;
    int cantidad_instrucciones_new;
    char en_new;
}MetricasDTB;

void desempaquetar_dtb(MensajeDinamico*, DTB*);
MensajeDinamico* generar_mensaje_dtb(int, DTB *);
DTB* encontrar_dtb_en_lista(t_list*, int, bool);
MetricasDTB* encontrar_metricas_en_lista(t_list*, int, bool);
void destruir_archivo_abierto(void*);

#endif //DTB_H
