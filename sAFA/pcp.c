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
    CPU* cpu_seleccionado;
    int tamanio_lista_cpus = list_size(lista_cpus);

    for(int i = 0; i<tamanio_lista_cpus; i++){
        cpu_seleccionado = list_get(lista_cpus, i);

        if(!(cpu_seleccionado->cantidad_procesos_asignados))
            return cpu_seleccionado;
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
PCP* inicializar_pcp(int algoritmo_planificador, int quantum){
    PCP* nuevo_pcp = malloc(sizeof(PCP));
    DTB* dtb_dummy;
    nuevo_pcp->cola_ready = queue_create();
    nuevo_pcp->lista_block = list_create();
    nuevo_pcp->lista_exec = list_create();
    sem_init(&(nuevo_pcp->semaforo_ready), 0, 0);
    pthread_mutex_init(&(nuevo_pcp->mutex_ready), NULL);
    pthread_mutex_init(&(nuevo_pcp->mutex_block), NULL);
    pthread_mutex_init(&(nuevo_pcp->mutex_exec), NULL);
    nuevo_pcp->algoritmo_planificacion = algoritmo_planificador;
    nuevo_pcp->quantum = quantum;
    nuevo_pcp->logger = log_create("safa.log", "PCP", true, log_level_from_string("info"));

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

/*!
 * Destruye el PCP liberando memoria
 * @param pcp_a_destruir Puntero al PCP a destruir
 */
void destruir_pcp(PCP* pcp_a_destruir){
    queue_destroy_and_destroy_elements(pcp_a_destruir->cola_ready, destruir_dtb);
    list_destroy_and_destroy_elements(pcp_a_destruir->lista_block, destruir_dtb);
    list_destroy_and_destroy_elements(pcp_a_destruir->lista_exec, destruir_dtb);

    pthread_mutex_destroy(&(pcp_a_destruir->mutex_ready));
    pthread_mutex_destroy(&(pcp_a_destruir->mutex_block));
    pthread_mutex_destroy(&(pcp_a_destruir->mutex_exec));
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
    for(int i = 0; i<list_size(pcp->lista_block); i++){
        dtb_seleccionado = list_get(pcp->lista_block, i);

        if(dtb_seleccionado->id == id_DTB){

            log_info(pcp->logger, "Pasando DTB %d a READY", id_DTB);
            agregar_a_ready(pcp, dtb_seleccionado);
            list_remove(pcp->lista_block, i);
            sem_post(&(pcp->semaforo_ready));
        }
    }
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
    dtb->status = READY;

    pthread_mutex_lock(&(pcp->mutex_ready));
    queue_push(pcp->cola_ready, dtb);
    pthread_mutex_unlock(&(pcp->mutex_ready));
}

/*!
 * Toma un DTB de ready y lo pasa a exec
 * @param pcp PCP que contiene las colas ready/exec
 * @return el DTB que se paso a exec
 */
DTB* ready_a_exec(PCP* pcp){

    pthread_mutex_lock(&(pcp->mutex_ready));
    DTB* dtb_seleccionado = queue_pop(pcp->cola_ready);
    pthread_mutex_unlock(&(pcp->mutex_ready));

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
}

/*!
 * Saca a un DTB de exec y lo retorna
 * @param pcp PCP que contiene la lista exec
 * @param id ID del DTB a sacar
 * @return el DTB seleccionado o NULL si no se encuentra un DTB con el ID pasado
 */
DTB* tomar_de_exec(PCP* pcp, int id){
    DTB* dtb_seleccionado;
    int size_lista = list_size(pcp->lista_exec);

    pthread_mutex_lock(&(pcp->mutex_exec));
    for(int i = 0; i<size_lista; i++){
        dtb_seleccionado = list_get(pcp->lista_exec, i);

        if(dtb_seleccionado->id == id){
            list_remove(pcp->lista_exec, i);
            pthread_mutex_unlock(&(pcp->mutex_exec));
            return dtb_seleccionado;
        }
    }
    pthread_mutex_unlock(&(pcp->mutex_exec));
    return NULL;
}

/*!
 * Saca a un DTB de exec y lo actualiza con datos recibidos del CPU luego de una ejecucion de ese DTB
 * @param pcp PCP que contiene la lista exec
 * @param datos_actualizados_dtb Nuevos datos a copiar al DTB
 * @return el DTB sacado de exec actualizado con los nuevos datos pasados
 */
DTB* conseguir_y_actualizar_dtb(PCP* pcp, DTB* datos_actualizados_dtb){
    DTB* dtb_seleccionado = tomar_de_exec(pcp, datos_actualizados_dtb->id);

    list_add_all(dtb_seleccionado->archivos_abiertos, datos_actualizados_dtb->archivos_abiertos);
    list_destroy(datos_actualizados_dtb->archivos_abiertos);

    dtb_seleccionado->status = datos_actualizados_dtb->status;
    dtb_seleccionado->program_counter = datos_actualizados_dtb->program_counter;

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

    while(correr){
        // TODO switch algoritmo planificador, por ahora es FIFO

        sem_wait(&(pcp->semaforo_ready));
        if(errno == EINTR)
            break;

        log_info(pcp->logger, "Esperando CPU disponible...");
        sem_wait(&cantidad_cpus);
        if(errno == EINTR)
            break;

        dtb_seleccionado = ready_a_exec(pcp);

        printf("Valor id dtb: %d\n", dtb_seleccionado->id);
        printf("Path script: %s\n", dtb_seleccionado->path_script);
        printf("Valor flag inicializado: %d\n", dtb_seleccionado->inicializado);

        cpu_seleccionado = seleccionar_cpu(conexiones_activas.lista_cpus);

        log_info(pcp->logger, "Enviando datos DTB %d a CPU %d", dtb_seleccionado->id, cpu_seleccionado->id);

        enviar_datos_dtb(cpu_seleccionado->socket, dtb_seleccionado);
        (cpu_seleccionado->cantidad_procesos_asignados)++;
    }

    return NULL;
}