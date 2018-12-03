//
// Created by utnso on 26/09/18.
//

#include "plp.h"

extern PCP* pcp;
extern t_log* logger;
extern bool correr;

/*!
 * Encuentra y aborta un DTB, informando error y liberando memoria
 * @param plp
 * @param id_dtb
 * @param codigo_error
 * @param archivos_abiertos
 */
void abortar_dtb(PLP* plp, int id_dtb, int codigo_error, t_dictionary* archivos_abiertos){
    DTB* dtb_seleccionado;
    codigo_error = abs(codigo_error);

    dtb_seleccionado = obtener_dtb_de_new(plp, id_dtb, true);

    if(dtb_seleccionado == NULL)
        dtb_seleccionado = obtener_dtb_de_exec(pcp, id_dtb);
    if(dtb_seleccionado == NULL)
        dtb_seleccionado = obtener_dtb_de_block(pcp, id_dtb);

    if(dtb_seleccionado == NULL){
        log_error(pcp->logger, "Falla al abortar DTB %d", id_dtb);
        return;
    }
    abortar_dtb_seleccionado(plp, dtb_seleccionado, codigo_error, archivos_abiertos);
}

/*!
 * Aborta un DTB ya seleccionado, informando error y liberando memoria
 * @param plp
 * @param dtb_seleccionado
 * @param codigo_error
 * @param archivos_abiertos
 */
void abortar_dtb_seleccionado(PLP* plp, DTB* dtb_seleccionado, int codigo_error, t_dictionary* archivos_abiertos){
    char* string_error = codigo_error_a_string(codigo_error);
    ArchivoAbierto* archivo;

    log_warning(plp->logger, "DTB %d (Path: %s) produjo un error, abortando (%d: %s)", dtb_seleccionado->id,
                dtb_seleccionado->path_script, codigo_error, string_error);

    for(int i = 0; i<list_size(dtb_seleccionado->archivos_abiertos); i++){
        archivo = list_get(dtb_seleccionado->archivos_abiertos, i);
        dictionary_remove(archivos_abiertos, archivo->path);
    }

    printf("El DTB %d fue abortado por error, revisar log para mas informacion\n", dtb_seleccionado->id);
    destruir_dtb(dtb_seleccionado);
    sem_post(&plp->semaforo_multiprogramacion);
    free(string_error);
}

/*!
 * Crea e inicializa un PLP
 * @param multiprogramacion Cuantos procesos puede correr sistema al mismo tiempo
 * @return el PLP creado
 */
PLP* inicializar_plp(int multiprogramacion, char* logger_level, int logger_consola){
    PLP* nuevo_plp = malloc(sizeof(PLP));
    nuevo_plp->lista_new = list_create();
    sem_init(&(nuevo_plp->semaforo_new), 0, 0);
    sem_init(&(nuevo_plp->semaforo_multiprogramacion), 0, (unsigned int)multiprogramacion);
    pthread_mutex_init(&(nuevo_plp->mutex_new), NULL);
    pthread_mutex_init(&(nuevo_plp->mutex_pausa), NULL);
    pthread_mutex_init(&(nuevo_plp->mutex_metricas), NULL);
    nuevo_plp->logger = log_create("safa.log", "PLP", (bool)logger_consola,
            log_level_from_string(logger_level));
    nuevo_plp->metricas_dtbs = list_create();

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
    pthread_mutex_destroy(&(plp_a_destruir->mutex_pausa));
    pthread_mutex_destroy(&(plp_a_destruir->mutex_metricas));
    list_destroy_and_destroy_elements(plp_a_destruir->metricas_dtbs, free);
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
 * Pasa un DTB de NEW a READY
 * @param plp PLP que contiene la lista NEW
 * @param id_dtb ID del DTB a pasar a READY
 */
void pasar_new_a_ready(PLP* plp, int id_dtb){
    DTB* dtb_seleccionado;
    MetricasDTB* metricas;

    pthread_mutex_lock(&plp->mutex_new);
    dtb_seleccionado = encontrar_dtb_en_lista(plp->lista_new, id_dtb, true);

    if(dtb_seleccionado != NULL){
        log_info(logger, "Pasando DTB %d de NEW a READY...", id_dtb);
        metricas = encontrar_metricas_en_lista(plp->metricas_dtbs, id_dtb, false);
        metricas->en_new = 0;
        agregar_a_ready(pcp, dtb_seleccionado);
    }else{
        log_error(plp->logger, "DTB %d no encontrado en NEW", id_dtb);
    }
    pthread_mutex_unlock(&plp->mutex_new);
}

/*!
 * Imprime el estado de la lista NEW
 * @param plp PLP que contiene la lista NEW
 */
void imprimir_estado_plp(PLP* plp){
    DTB* dtb_seleccionado;
    int lista_size;

    pthread_mutex_lock(&(plp->mutex_new));
    lista_size = list_size(plp->lista_new);

    printf("----------------\n");
    printf("Estado cola NEW: %d DTBs\n", lista_size);

    if(lista_size) {
        printf("IDs: ");

        for (int i = 0; i < lista_size; i++) {
            dtb_seleccionado = list_get(plp->lista_new, i);
            printf("%d, ", dtb_seleccionado->id);
        }
        printf("\n");
    }
    printf("----------------\n");
    pthread_mutex_unlock(&(plp->mutex_new));
}

/*!
 * Encuentra y retorna un DTB de NEW
 * @param plp
 * @param id_dtb
 * @param eliminar eliminar al DTB de NEW
 * @return
 */
DTB* obtener_dtb_de_new(PLP *plp, int id_dtb, bool eliminar){
    DTB* dtb_seleccionado;

    pthread_mutex_lock(&(plp->mutex_new));
    dtb_seleccionado = encontrar_dtb_en_lista(plp->lista_new, id_dtb, eliminar);
    pthread_mutex_unlock(&(plp->mutex_new));
    if(dtb_seleccionado == NULL)
        log_trace(plp->logger, "DTB %d no encontrado en NEW", id_dtb);

    return dtb_seleccionado;
}

/*!
 * Actualiza todas las metricas de DTB sumando la cantidad de instrucciones ejecutadas pasadas por parametro
 * @param plp
 * @param cantidad_instrucciones_ejecutadas
 */
void actualizar_cantidad_instrucciones_en_new(PLP* plp, int cantidad_instrucciones_ejecutadas){
    int lista_size;
    MetricasDTB* metricas;

    lista_size = list_size(plp->metricas_dtbs);
    for(int i = 0; i<lista_size; i++){
        metricas = list_get(plp->metricas_dtbs, i);

        if(metricas->en_new)
            metricas->cantidad_instrucciones_new += cantidad_instrucciones_ejecutadas;
    }
}

/*!
 * Toma un int codigo de error y devuelve un string con la descripcion
 * @param codigo_error
 * @return
 */
char* codigo_error_a_string(int codigo_error){
    char* string_error = string_new();

    switch(codigo_error){
        case -1:
            string_append(&string_error, "Cantidad de argumentos erroneos en una linea (detalle en cpu.log)\n");
            break;

        case -2:
            string_append(&string_error, "Instruccion no reconocida (detalle en cpu.log)\n");
            break;

        case -3:
            string_append(&string_error, "Flag inicializado de DTB invalida\n");
            break;

        case -4:
            string_append(&string_error, "Un mensaje de comunicacion de una instruccion de CPU no fue enviado correctamente\n");
            break;

        case 10001:
            string_append(&string_error, "Path inexistente en instruccion abrir\n");
            break;

        case 10002:
            string_append(&string_error, "Espacio insuficiente en FM9 para instruccion abrir\n");
            break;

        case 10003:
            string_append(&string_error, "Se intento abrir un archivo ya abierto\n");
            break;

        case 20001:
            string_append(&string_error, "El archivo no se encuentra abierto para instruccion asignar\n");
            break;

        case 20002:
            string_append(&string_error, "Fallo de segmento/memoria en instruccion asignar\n");
            break;

        case 20003:
            string_append(&string_error, "Espacio insuficiente para instruccion asignar\n");
            break;

        case 30001:
            string_append(&string_error, "El archivo no se encuentra abierto para instruccion flush\n");
            break;

        case 30002:
            string_append(&string_error, "Fallo de segmento/memoria para instruccion flush\n");
            break;

        case 30003:
            string_append(&string_error, "Espacio insuficiente en MDJ para instruccion flush\n");
            break;

        case 30004:
            string_append(&string_error, "El archivo no existe en MDJ para instruccion flush\n");
            break;

        case 40001:
            string_append(&string_error, "El archivo no se encuentra abierto para instruccion close\n");
            break;

        case 40002:
            string_append(&string_error, "Fallo de segmento/memoria para instruccion close\n");
            break;

        case 50001:
            string_append(&string_error, "Archivo ya existente para instruccion crear\n");
            break;

        case 50002:
            string_append(&string_error, "Espacio insuficiente en MDJ para instruccion crear\n");
            break;

        case 60001:
            string_append(&string_error, "Archivo no existe para instruccion borrar\n");
            break;

        default:
            string_append(&string_error, "Codigo de error no reconocido\n");
            break;
    }
    return string_error;
}

/*!
 * Ejecuta el PLP
 * @param arg PLP a ejecutar
 * @return NULL cuando se recibe exit desde consola
 */
void* ejecutar_plp(void* arg){
    PLP* plp = (PLP*)arg;
    DTB* dtb_seleccionado = NULL;

    while(correr){
        log_info(plp->logger, "Esperando nuevo DTB en NEW...");
        sem_wait(&(plp->semaforo_new));
        if(errno == EINTR)
            break;

        pthread_mutex_lock(&(plp->mutex_new));
        for(int i = 0; i<list_size(plp->lista_new); i++) {
            dtb_seleccionado = list_get(plp->lista_new, i);

            if(!dtb_seleccionado->cargando)
                break;
        }
        pthread_mutex_unlock(&(plp->mutex_new));

        //pthread_mutex_lock(&plp->mutex_pausa);

        log_info(plp->logger, "Esperando semaforo multiprogramacion...");
        sem_wait(&(plp->semaforo_multiprogramacion));
        if(errno == EINTR)
            break;

        dtb_seleccionado->cargando = 1;
        desbloquear_dtb_dummy(pcp, dtb_seleccionado->id, dtb_seleccionado->path_script);

        //pthread_mutex_unlock(&plp->mutex_pausa);
    }

    return NULL;
}
