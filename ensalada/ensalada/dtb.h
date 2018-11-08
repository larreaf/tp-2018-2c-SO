#ifndef DTB_H
#define DTB_H

#include <commons/collections/list.h>
#include "mensaje.h"

typedef enum {
    BLOQUEAR,
    READY,
    DTB_EXIT,
}t_accion_post_instruccion;

typedef struct{
    char* path;
    int direccion_memoria;
}ArchivoAbierto;

typedef struct{
    int id;
    char* path_script;
    int program_counter;
    int inicializado;
    int quantum;
    t_accion_post_instruccion status;
    t_list* archivos_abiertos;
}DTB;

void desempaquetar_dtb(MensajeDinamico*, DTB*);
void enviar_datos_dtb(int, DTB*);
DTB* encontrar_dtb_en_lista(t_list*, int, bool);

#endif //DTB_H
