//
// Created by utnso on 26/09/18.
//

#include "pcp.h"

extern ConexionesActivas conexiones_activas;
extern sem_t cantidad_cpus;
extern bool correr;

/*!
 * Busca un CPU libre y lo retorna a fin de ejecutar un script en el mismo
 * @param lista_cpus lista de CPUs conectados
 * @return CPU libre
 */
CPU* seleccionar_cpu(t_list* lista_cpus){
    CPU* cpu_seleccionado = NULL;
    int tamanio_lista_cpus = list_size(lista_cpus);

    for(int i = 0; i<tamanio_lista_cpus; i++){
        cpu_seleccionado = list_get(lista_cpus, i);

        if(!(cpu_seleccionado->cantidad_procesos_asignados)) {
            cpu_seleccionado->id = i;
            return cpu_seleccionado;
        }
    }
}

/*!
 * Busca el CPU relacionado a un socket y lo marca como libre
 * @param conexiones_activas ConexionesActivas que contiene la lista de CPUs
 * @param socket socket relacionado al CPU que se quiere marcar como libre
 */
void decrementar_procesos_asignados_cpu(ConexionesActivas conexiones_activas, int socket){
    int size_lista = list_size(conexiones_activas.lista_cpus);
    CPU* cpu_seleccionado;

    for(int i = 0; i<size_lista; i++){
        cpu_seleccionado = list_get(conexiones_activas.lista_cpus, i);

        if(cpu_seleccionado->socket ==  socket) {
            cpu_seleccionado->cantidad_procesos_asignados--;
            break;
        }
    }
}

/*!
 * Crea e inicializa el PCP creando todas las estructuras necesarias
 * @param algoritmo_planificador RR o VRR
 * @param quantum quantum con el cual planificar
 * @return PCP inicializado
 */
PCP* inicializar_pcp(int algoritmo_planificador, int quantum, int retardo, char* logger_level, int logger_consola,
        int cantidad_lineas_equipo_grande){
    PCP* nuevo_pcp = malloc(sizeof(PCP));
    DTB* dtb_dummy;
    nuevo_pcp->cola_ready = queue_create();
    nuevo_pcp->cola_ready_aux = queue_create();
    nuevo_pcp->lista_block = list_create();
    nuevo_pcp->lista_exec = list_create();
    sem_init(&(nuevo_pcp->semaforo_ready), 0, 0);
    sem_init(&(nuevo_pcp->semaforo_dummy), 0, 0);
    pthread_mutex_init(&(nuevo_pcp->mutex_ready), NULL);
    pthread_mutex_init(&(nuevo_pcp->mutex_ready_aux), NULL);
    pthread_mutex_init(&(nuevo_pcp->mutex_block), NULL);
    pthread_mutex_init(&(nuevo_pcp->mutex_exec), NULL);
    pthread_mutex_init(&(nuevo_pcp->mutex_config), NULL);
    pthread_mutex_init(&(nuevo_pcp->mutex_pausa), NULL);
    nuevo_pcp->algoritmo_planificacion = algoritmo_planificador;
    nuevo_pcp->quantum = quantum;
    nuevo_pcp->logger = log_create("safa.log", "PCP", (bool)logger_consola, log_level_from_string(logger_level));
    nuevo_pcp->retardo_planificacion = retardo;
    nuevo_pcp->finalizar_dtb = 0;
    nuevo_pcp->recursos = dictionary_create();
    nuevo_pcp->cantidad_lineas_equipo_grande = cantidad_lineas_equipo_grande;

    dtb_dummy = crear_dtb(0, 0);
    agregar_a_block(nuevo_pcp, dtb_dummy);

    return nuevo_pcp;
}

/*!
 * Destruye un DTB liberando memoria
 * @param arg Puntero al DTB a destruir
 */
void destruir_dtb(void* arg){
    DTB* dtb = (DTB*)arg;

    free(dtb->path_script);
    list_destroy_and_destroy_elements(dtb->archivos_abiertos, free);
    free(dtb);
}

void destruir_recurso(void *arg){
    Recurso* recurso_seleccionado = (Recurso*)arg;

    queue_destroy(recurso_seleccionado->cola);
    free(recurso_seleccionado);
}

/*!
 * Destruye el PCP liberando memoria
 * @param pcp_a_destruir Puntero al PCP a destruir
 */
void destruir_pcp(PCP* pcp_a_destruir){
    queue_destroy_and_destroy_elements(pcp_a_destruir->cola_ready, destruir_dtb);
    queue_destroy_and_destroy_elements(pcp_a_destruir->cola_ready_aux, destruir_dtb);
    list_destroy_and_destroy_elements(pcp_a_destruir->lista_block, destruir_dtb);
    list_destroy_and_destroy_elements(pcp_a_destruir->lista_exec, destruir_dtb);
    sem_destroy(&pcp_a_destruir->semaforo_ready);
    sem_destroy(&pcp_a_destruir->semaforo_dummy);
    dictionary_destroy_and_destroy_elements(pcp_a_destruir->recursos, destruir_recurso);

    pthread_mutex_destroy(&(pcp_a_destruir->mutex_ready));
    pthread_mutex_destroy(&(pcp_a_destruir->mutex_ready_aux));
    pthread_mutex_destroy(&(pcp_a_destruir->mutex_block));
    pthread_mutex_destroy(&(pcp_a_destruir->mutex_exec));
    pthread_mutex_destroy(&(pcp_a_destruir->mutex_config));
    pthread_mutex_destroy(&(pcp_a_destruir->mutex_pausa));
}

/*!
 * Crea un DTB
 * @param id ID del DTB a crear
 * @param inicializado Flag inicializado (0 si es dummy, 1 si es un DTB normal)
 * @return Puntero al DTB creado
 */
DTB* crear_dtb(int id, int inicializado){
    DTB* nuevo_dtb = malloc(sizeof(DTB));
    nuevo_dtb->inicializado = inicializado;
    nuevo_dtb->id = id;
    nuevo_dtb->archivos_abiertos = list_create();
    nuevo_dtb->path_script = string_new();
    nuevo_dtb->program_counter = 0;
    nuevo_dtb->status = READY;
    nuevo_dtb->quantum = 0;

    return nuevo_dtb;
}

/*!
 * Vuelve el DTB dummy a cero una vez que ya se utilizo para cargar un script
 * @param dtb_dummy Puntero al DTB dummy
 */
void resetear_dtb_dummy(DTB* dtb_dummy){
    if(string_length(dtb_dummy->path_script)) {
        free(dtb_dummy->path_script);
        dtb_dummy->path_script = string_new();
    }

    dtb_dummy->id = 0;
    dtb_dummy->status = BLOQUEAR;
}

/*!
 * Pasa un DTB de block a ready
 * @param pcp PCP que contiene las listas block/ready y al DTB en block
 * @param id_DTB ID del DTB a desbloquear
 */
void desbloquear_dtb(PCP* pcp, int id_DTB){
    DTB* dtb_seleccionado;

    pthread_mutex_lock(&(pcp->mutex_block));
    dtb_seleccionado = encontrar_dtb_en_lista(pcp->lista_block, id_DTB, true);

    if(dtb_seleccionado != NULL) {
        if (dtb_seleccionado->quantum && pcp->algoritmo_planificacion == VRR) {
            log_info(pcp->logger, "Pasando DTB %d a READY AUX", id_DTB);
            agregar_a_ready_aux(pcp, dtb_seleccionado);
        } else {
            log_info(pcp->logger, "Pasando DTB %d a READY", id_DTB);
            agregar_a_ready(pcp, dtb_seleccionado);
        }
    } else
        log_error(pcp->logger, "DTB %d no encontrado en BLOCK", id_DTB);

    pthread_mutex_unlock(&(pcp->mutex_block));
}

DTB* obtener_dtb_de_block(PCP* pcp, int id_dtb){
    DTB* dtb_seleccionado;

    pthread_mutex_lock(&(pcp->mutex_block));
    dtb_seleccionado = encontrar_dtb_en_lista(pcp->lista_block, id_dtb, true);

    if(dtb_seleccionado == NULL)
        log_trace(pcp->logger, "DTB %d no encontrado en BLOCK", id_dtb);

    pthread_mutex_unlock(&(pcp->mutex_block));

    return dtb_seleccionado;
}

void desbloquear_dtb_cargando_archivo(PCP* pcp, int id_DTB, char* path, int direccion_archivo, int cant_lineas){
    DTB* dtb_seleccionado;
    ArchivoAbierto* archivo_a_cargar = malloc(sizeof(ArchivoAbierto));

    archivo_a_cargar->path = string_new();
    string_append(&archivo_a_cargar->path, path);
    archivo_a_cargar->direccion_memoria = direccion_archivo;

    pthread_mutex_lock(&(pcp->mutex_block));
    dtb_seleccionado = encontrar_dtb_en_lista(pcp->lista_block, id_DTB, true);

    if(dtb_seleccionado != NULL){
        log_info(pcp->logger, "Cargando archivo %s en lista de archivos de DTB %d (direccion %d)",
                 archivo_a_cargar->path, id_DTB, direccion_archivo);

        list_add(dtb_seleccionado->archivos_abiertos, archivo_a_cargar);
        if(pcp->algoritmo_planificacion == PROPIO && cant_lineas >= pcp->cantidad_lineas_equipo_grande)
            archivo_a_cargar->equipo_grande = 1;
        else
            archivo_a_cargar->equipo_grande = 0;

        if (dtb_seleccionado->quantum && pcp->algoritmo_planificacion == VRR) {
            agregar_a_ready_aux(pcp, dtb_seleccionado);
        } else {
            agregar_a_ready(pcp, dtb_seleccionado);
        }
    } else
        log_error(pcp->logger, "DTB %d no encontrado en BLOCK", id_DTB);

    pthread_mutex_unlock(&(pcp->mutex_block));
}

/*!
 * Carga datos de un DTB en DTB dummy y lo pasa de block a ready
 * @param pcp PCP que contiene las listas block/ready y el DTB dummy
 * @param id_DTB ID del DTB a cargar en el DTB dummy
 * @param path_script path al script a cargar con el DTB dummy
 */
void desbloquear_dtb_dummy(PCP* pcp, int id_DTB, char* path_script){
    DTB* dtb_seleccionado;
    sem_wait(&pcp->semaforo_dummy);

    for(int i = 0; i<list_size(pcp->lista_block); i++){
        dtb_seleccionado = list_get(pcp->lista_block, i);

        if(!(dtb_seleccionado->inicializado)){
            log_info(pcp->logger, "Desbloqueando DTB dummy");
            dtb_seleccionado->id = id_DTB;
            string_append(&(dtb_seleccionado->path_script), path_script);
            desbloquear_dtb(pcp, id_DTB);
        }
    }
}

/*!
 * Agrega un DTB a ready
 * @param pcp PCP que contiene la cola ready
 * @param dtb DTB a agregar a ready
 */
void agregar_a_ready(PCP* pcp, DTB* dtb){
    ArchivoAbierto* archivo_seleccionado;
    int tamanio_lista_archivos;

    dtb->status = READY;
    dtb->quantum = pcp->quantum;

    if(pcp->algoritmo_planificacion != PROPIO) {
        log_info(pcp->logger, "Pasando DTB %d a READY", dtb->id);
        pthread_mutex_lock(&(pcp->mutex_ready));
        queue_push(pcp->cola_ready, dtb);
        pthread_mutex_unlock(&(pcp->mutex_ready));
    }else{
        tamanio_lista_archivos = list_size(dtb->archivos_abiertos);

        for(int i = 0; i < tamanio_lista_archivos; i++){
            archivo_seleccionado = list_get(dtb->archivos_abiertos, i);

            if(archivo_seleccionado->equipo_grande){
                log_info(pcp->logger, "Pasando DTB %d a READY AUX", dtb->id);
                pthread_mutex_lock(&(pcp->mutex_ready_aux));
                queue_push(pcp->cola_ready_aux, dtb);
                pthread_mutex_unlock(&(pcp->mutex_ready_aux));
                sem_post(&(pcp->semaforo_ready));
                return;
            }
        }
        pthread_mutex_lock(&(pcp->mutex_ready));
        queue_push(pcp->cola_ready, dtb);
        pthread_mutex_unlock(&(pcp->mutex_ready));
    }
    sem_post(&(pcp->semaforo_ready));
}

/*!
 * Agrega un DTB a ready aux
 * @param pcp PCP que contiene la cola ready aux
 * @param dtb DTB a agregar a ready aux
 */
void agregar_a_ready_aux(PCP* pcp, DTB* dtb){
    dtb->status = READY;

    pthread_mutex_lock(&(pcp->mutex_ready_aux));
    queue_push(pcp->cola_ready_aux, dtb);
    pthread_mutex_unlock(&(pcp->mutex_ready_aux));
    sem_post(&(pcp->semaforo_ready));
}

/*!
 * Toma un DTB de ready y lo pasa a exec
 * @param pcp PCP que contiene las colas ready/exec
 * @return el DTB que se paso a exec
 */
DTB* ready_a_exec(PCP* pcp){
    DTB* dtb_seleccionado = NULL;

    pthread_mutex_lock(&(pcp->mutex_ready));
    pthread_mutex_lock(&(pcp->mutex_ready_aux));

    if(pcp->algoritmo_planificacion == RR)
        dtb_seleccionado = queue_pop(pcp->cola_ready);
    else{
        if(!queue_is_empty(pcp->cola_ready_aux))
            dtb_seleccionado = queue_pop(pcp->cola_ready_aux);
        else
            dtb_seleccionado = queue_pop(pcp->cola_ready);
    }

    pthread_mutex_unlock(&(pcp->mutex_ready));
    pthread_mutex_unlock(&(pcp->mutex_ready_aux));

    pthread_mutex_lock(&(pcp->mutex_exec));
    list_add(pcp->lista_exec, dtb_seleccionado);
    pthread_mutex_unlock(&(pcp->mutex_exec));
    return dtb_seleccionado;
}

/*!
 * Agrega un DTB a la lista block
 * @param pcp PCP que contiene a la lista block
 * @param dtb DTB a pasar a block
 */
void agregar_a_block(PCP* pcp, DTB* dtb){
    dtb->status = BLOQUEAR;

    if(!dtb->inicializado) {
        resetear_dtb_dummy(dtb);
        log_info(pcp->logger, "Bloqueando DTB dummy");
    }else
        log_info(pcp->logger, "Bloqueando DTB %d", dtb->id);

    pthread_mutex_lock(&(pcp->mutex_block));
    list_add(pcp->lista_block, dtb);
    pthread_mutex_unlock(&(pcp->mutex_block));

    if(!dtb->inicializado)
        sem_post(&pcp->semaforo_dummy);
}

/*!
 * Saca a un DTB de exec y lo retorna
 * @param pcp PCP que contiene la lista exec
 * @param id ID del DTB a sacar
 * @return el DTB seleccionado o NULL si no se encuentra un DTB con el ID pasado
 */
DTB* obtener_dtb_de_exec(PCP *pcp, int id){
    DTB* dtb_seleccionado;

    pthread_mutex_lock(&(pcp->mutex_exec));
    dtb_seleccionado = encontrar_dtb_en_lista(pcp->lista_exec, id, true);

    if(dtb_seleccionado == NULL)
        log_trace(pcp->logger, "DTB %d no encontrado en EXEC", id);

    pthread_mutex_unlock(&(pcp->mutex_exec));
    return dtb_seleccionado;
}

/*!
 * Saca a un DTB de exec y lo actualiza con datos recibidos del CPU luego de una ejecucion de ese DTB
 * @param pcp PCP que contiene la lista exec
 * @param datos_actualizados_dtb Nuevos datos a copiar al DTB
 * @return el DTB sacado de exec actualizado con los nuevos datos pasados
 */
DTB* conseguir_y_actualizar_dtb(PCP* pcp, DTB* datos_actualizados_dtb){
    DTB* dtb_seleccionado = obtener_dtb_de_exec(pcp, datos_actualizados_dtb->id);

    list_add_all(dtb_seleccionado->archivos_abiertos, datos_actualizados_dtb->archivos_abiertos);
    list_destroy(datos_actualizados_dtb->archivos_abiertos);

    dtb_seleccionado->status = datos_actualizados_dtb->status;
    dtb_seleccionado->program_counter = datos_actualizados_dtb->program_counter;
    dtb_seleccionado->quantum = datos_actualizados_dtb->quantum;

    return dtb_seleccionado;
}

/*!
 * Imprime el estado de todas las colas
 * @param pcp PCP* para imprimir estado
 */
void imprimir_estado_pcp(PCP* pcp){
    DTB* dtb_seleccionado;
    int lista_size;

    pthread_mutex_lock(&(pcp->mutex_ready));
    lista_size = queue_size(pcp->cola_ready);

    printf("Estado cola READY: %d DTBs\n", lista_size);
    printf("----------------\n");

    pthread_mutex_unlock(&(pcp->mutex_ready));

    if(pcp->algoritmo_planificacion == VRR){
        pthread_mutex_lock(&(pcp->mutex_ready_aux));
        lista_size = queue_size(pcp->cola_ready_aux);

        printf("Estado cola READY AUX: %d DTBs\n", lista_size);
        printf("----------------\n");

        pthread_mutex_unlock(&(pcp->mutex_ready_aux));
    }


    pthread_mutex_lock(&(pcp->mutex_block));
    lista_size = list_size(pcp->lista_block);
    printf("Estado lista BLOCK: %d DTBs\n", lista_size);

    if(lista_size) {
        printf("IDs: ");

        for (int i = 0; i < lista_size; i++) {
            dtb_seleccionado = list_get(pcp->lista_block, i);
            printf("%d, ", dtb_seleccionado->id);
        }
        printf("\n");
    }
    printf("----------------\n");

    pthread_mutex_unlock(&(pcp->mutex_block));

    pthread_mutex_lock(&(pcp->mutex_exec));
    lista_size = list_size(pcp->lista_exec);
    printf("Estado lista EXEC: %d DTBs\n", lista_size);

    if(lista_size) {
        printf("IDs: ");

        for (int i = 0; i < lista_size; i++) {
            dtb_seleccionado = list_get(pcp->lista_exec, i);
            printf("%d, ", dtb_seleccionado->id);
        }
        printf("\n");
    }
    printf("----------------\n");
    pthread_mutex_unlock(&(pcp->mutex_exec));
}

DTB* encontrar_dtb_pcp(PCP* pcp, int id_dtb){
    DTB* dtb_seleccionado;

    pthread_mutex_lock(&pcp->mutex_block);
    dtb_seleccionado = encontrar_dtb_en_lista(pcp->lista_block, id_dtb, false);
    pthread_mutex_unlock(&pcp->mutex_block);

    if(dtb_seleccionado != NULL)
        return dtb_seleccionado;

    pthread_mutex_lock(&pcp->mutex_exec);
    dtb_seleccionado = encontrar_dtb_en_lista(pcp->lista_exec, id_dtb, false);
    pthread_mutex_unlock(&pcp->mutex_exec);

    return dtb_seleccionado;
}

/*!
 * Ejecuta el PCP
 * @param arg Puntero al PCP a ejecutar
 * @return NULL cuando se recibe exit desde consola
 */
void* ejecutar_pcp(void* arg){
    PCP* pcp = (PCP*)arg;
    DTB* dtb_seleccionado;
    CPU* cpu_seleccionado;
    MensajeDinamico* mensaje_dtb;

    while(correr){
        pthread_mutex_lock(&pcp->mutex_pausa);

        sem_wait(&(pcp->semaforo_ready));
        if(errno == EINTR)
            break;

        pthread_mutex_lock(&pcp->mutex_config);

        log_info(pcp->logger, "Esperando CPU disponible...");
        sem_wait(&cantidad_cpus);
        if(errno == EINTR)
            break;

        dtb_seleccionado = ready_a_exec(pcp);
        cpu_seleccionado = seleccionar_cpu(conexiones_activas.lista_cpus);

        if(dtb_seleccionado->inicializado)
            log_info(pcp->logger, "Enviando datos DTB %d a CPU %d", dtb_seleccionado->id, cpu_seleccionado->id);
        else
            log_info(pcp->logger, "Enviando datos DTB DUMMY a CPU %d para cargar script %s para DTB %d",
                    cpu_seleccionado->id, dtb_seleccionado->path_script, dtb_seleccionado->id);

        usleep((__useconds_t)(pcp->retardo_planificacion*1000));

        mensaje_dtb = generar_mensaje_dtb(cpu_seleccionado->socket, dtb_seleccionado);
        enviar_mensaje(mensaje_dtb);
        (cpu_seleccionado->cantidad_procesos_asignados)++;

        pthread_mutex_unlock(&pcp->mutex_config);
        pthread_mutex_unlock(&pcp->mutex_pausa);
    }

    return NULL;
}
