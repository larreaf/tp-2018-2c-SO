//
// Created by utnso on 26/09/18.
//

#include "plp.h"

extern PCP* pcp;
extern t_log* logger;

/*!
 * Crea e inicializa un PLP
 * @param multiprogramacion Cuantos procesos puede correr sistema al mismo tiempo
 * @return el PLP creado
 */
PLP* inicializar_plp(int multiprogramacion){
    PLP* nuevo_plp = malloc(sizeof(PLP));
    nuevo_plp->lista_new = list_create();
    sem_init(&(nuevo_plp->semaforo_new), 0, 0);
    sem_init(&(nuevo_plp->semaforo_multiprogramacion), 0, (unsigned int)multiprogramacion);
    pthread_mutex_init(&(nuevo_plp->mutex_new), NULL);
    nuevo_plp->logger = log_create("safa.log", "PLP", true, log_level_from_string("info"));

    return nuevo_plp;
}

/*!
 * Destruye PLP y libera memoria
 * @param plp_a_destruir PLP a destruir
 */
void destruir_plp(PLP* plp_a_destruir){
    pthread_mutex_lock(&(plp_a_destruir->mutex_ready));
    list_destroy_and_destroy_elements(plp_a_destruir->lista_new, destruir_dtb);
    pthread_mutex_unlock(&(plp_a_destruir->mutex_ready));
    pthread_mutex_destroy(&(plp_a_destruir->mutex_ready));
}

/*!
 * Agrega un DTB a new
 * @param plp PLP que contiene la lista NEW
 * @param dtb DTB a agregar a NEW
 */
void agregar_a_new(PLP* plp, DTB* dtb){
    log_info(plp->logger, "Pasando DTB %d a NEW", dtb->id);
    pthread_mutex_lock(&(plp->mutex_new));
    list_add(plp->lista_new, dtb);
    pthread_mutex_unlock(&(plp->mutex_new));
    sem_post(&(plp->semaforo_new));
}

/*!
 * Saca un PLP de NEW
 * @param plp PLP que contiene la lista NEW
 * @param id_dtb ID del DTB a sacar de NEW
 */
void eliminar_de_new(PLP *plp, int id_dtb){
    DTB* dtb_seleccionado;

    pthread_mutex_lock(&(plp->mutex_new));
    for(int i = 0; i<list_size(plp->lista_new); i++){
        dtb_seleccionado = list_get(plp->lista_new, i);

        if(dtb_seleccionado->id == id_dtb)
            list_remove(plp->lista_new, i);
    }
    pthread_mutex_unlock(&(plp->mutex_new));
}

/*!
 * Pasa un DTB de NEW a READY
 * @param plp PLP que contiene la lista NEW
 * @param id_dtb ID del DTB a pasar a READY
 */
void pasar_new_a_ready(PLP* plp, int id_dtb){
    DTB* dtb_seleccionado;

    // TODO sincronizar

    for(int i = 0; i<list_size(plp->lista_new); i++){
        dtb_seleccionado = list_get(plp->lista_new, i);

        if(dtb_seleccionado->id == id_dtb){
            log_info(logger, "Pasando DTB %d de NEW a READY...");
            agregar_a_ready(pcp, dtb_seleccionado);
            eliminar_de_new(plp, id_dtb);
            // TODO semaforo ready
        }
    }
}

/*!
 * Imprime el estado de la lista NEW
 * @param plp PLP que contiene la lista NEW
 */
void imprimir_estado(PLP* plp){
    pthread_mutex_lock(&(plp->mutex_new));
    int lista_size = list_size(plp->lista_new);
    DTB* dtb_seleccionado;

    printf("Estado cola NEW: %d DTBs\n", lista_size);

    if(lista_size) {
        printf("IDs: ");

        for (int i = 0; i < lista_size; i++) {
            dtb_seleccionado = list_get(plp->lista_new, i);
            printf("%d, ", dtb_seleccionado->id);
        }
        printf("\n");
    }
    pthread_mutex_unlock(&(plp->mutex_new));
}

/*!
 * Ejecuta el PLP
 * @param arg PLP a ejecutar
 * @return No retorna
 */
void* ejecutar_plp(void* arg){
    PLP* plp = (PLP*)arg;
    DTB* dtb_seleccionado;

    while(1){
        sem_wait(&(plp->semaforo_new));
        sem_wait(&(plp->semaforo_multiprogramacion));

        pthread_mutex_lock(&(plp->mutex_new));
        dtb_seleccionado = list_get(plp->lista_new, 0);
        pthread_mutex_unlock(&(plp->mutex_new));

        desbloquear_dtb_dummy(pcp, dtb_seleccionado->id, dtb_seleccionado->path_script);

        // TODO sacar esto y codear logica para elegir dtb de NEW y pasarlo a READY
        list_remove_and_destroy_element(plp->lista_new, 0, destruir_dtb);
    }
}
