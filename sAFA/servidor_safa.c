//
// Created by utnso on 25/09/18.
//

#include "servidor_safa.h"

extern ConexionesActivas conexiones_activas;
extern t_log* logger;
extern cfg_safa* configuracion;
extern PCP* pcp;
extern PLP* plp;
extern sem_t cantidad_cpus, arrancar_planificadores;
extern bool correr;
extern t_dictionary* archivos_abiertos;

/*!
 * Ejecuta el servidor de safa para comunicarse con CPUs y elDiego
 * @param arg No utilizado
 * @return NULL cuando se recibe exit desde consola
 */
void* ejecutar_servidor(void *arg){
    int conexiones_permitidas[cantidad_tipos_procesos] = {0}, id_dtb, direccion_archivo, codigo_error, resultado,
    cant_lineas, cantidad_instrucciones_ejecutadas, cantidad_instrucciones_dma, identificador_cpu;
    int* copia_id_dtb;
    MensajeDinamico* mensaje_respuesta, *mensaje;
    char* str = NULL, *path, *nombre_recurso;
    DTB datos_dtb;
    DTB* dtb_seleccionado;
    MetricasDTB* metricas;
    Recurso* recurso_seleccionado;

    conexiones_permitidas[t_elDiego] = 1;
    conexiones_permitidas[t_consola_safa] = 1;
    conexiones_permitidas[t_cpu] = 10;
    conexiones_activas = inicializar_conexiones_activas(logger, configuracion->ip,configuracion->puerto,
            conexiones_permitidas, t_safa);

    while (correr) {

        // bloquea hasta recibir un MensajeEntrante y lo retorna, ademas internamente maneja handshakes y desconexiones
        // sin retornar
        mensaje = esperar_mensajes(conexiones_activas);

        // cuando esperar_mensajes retorna, devuelve un MensajeEntrante que tiene como campos el socket que lo envio,
        // el header que se envio y el tipo de proceso que lo envio
        switch (mensaje->header) {
            case DATOS_DTB:
                // un CPU termino de ejecutar instrucciones y manda los datos del DTB actualizados

                desempaquetar_dtb(mensaje, &datos_dtb);
                recibir_int(&cantidad_instrucciones_ejecutadas, mensaje);
                recibir_int(&cantidad_instrucciones_dma, mensaje);

                if(datos_dtb.inicializado)
                    log_info(logger, "Recibidos datos de DTB %d", datos_dtb.id);
                else
                    log_info(logger, "Recibidos datos de DTB DUMMY");

                dtb_seleccionado = conseguir_y_actualizar_dtb(pcp, &datos_dtb);
                free(datos_dtb.path_script);
                decrementar_procesos_asignados_cpu(conexiones_activas, mensaje->socket);
                sem_post(&cantidad_cpus);

                pthread_mutex_lock(&plp->mutex_metricas);
                metricas = encontrar_metricas_en_lista(plp->metricas_dtbs, dtb_seleccionado->id, false);
                metricas->cantidad_instrucciones_ejecutadas += cantidad_instrucciones_ejecutadas;
                metricas->cantidad_instrucciones_dma += cantidad_instrucciones_dma;
                actualizar_cantidad_instrucciones_en_new(plp, cantidad_instrucciones_ejecutadas);
                pthread_mutex_unlock(&plp->mutex_metricas);

                if(pcp->finalizar_dtb && pcp->finalizar_dtb == datos_dtb.id){
                    log_info(logger, "DTB %d finalizado por comando, cerrando DTB", datos_dtb.id);
                    destruir_dtb(dtb_seleccionado);
                    sem_post(&plp->semaforo_multiprogramacion);
                    destruir_mensaje(mensaje);
                    pcp->finalizar_dtb = 0;
                    continue;
                }

                switch(datos_dtb.status){
                    case READY:
                        agregar_a_ready(pcp, dtb_seleccionado);
                        break;

                    case BLOQUEAR:
                        // los datos actualizados del DTB indican que debe ser bloqueado

                        agregar_a_block(pcp, dtb_seleccionado);
                        break;

                    case DTB_EXIT:
                        // los datos actualizados del DTB indican que debe ser pasado a exit
                        log_info(logger, "DTB %d devolvio EXIT, cerrando DTB", datos_dtb.id);
                        printf("DTB %d ha finalizado exitosamente\n", datos_dtb.id);
                        destruir_dtb(dtb_seleccionado);
                        sem_post(&plp->semaforo_multiprogramacion);
                        break;

                    default:
                        // los datos actualizados del DTB indican que produjo un error
                        abortar_dtb_seleccionado(plp, dtb_seleccionado, datos_dtb.status, archivos_abiertos);
                        break;
                }
                break;

            case PASAR_DTB_A_READY:
                recibir_int(&id_dtb, mensaje);
                pasar_new_a_ready(plp, id_dtb);
                break;

            case DESBLOQUEAR_DTB:
                recibir_int(&id_dtb, mensaje);
                desbloquear_dtb(pcp, id_dtb);
                break;

            case ABORTAR_DTB:
                recibir_int(&id_dtb, mensaje);
                recibir_int(&codigo_error, mensaje);
                abortar_dtb(plp, id_dtb, codigo_error, archivos_abiertos);
                break;

            case RESULTADO_CARGAR_ARCHIVO:
                recibir_int(&id_dtb, mensaje);
                recibir_string(&path, mensaje);
                recibir_int(&direccion_archivo, mensaje);
                recibir_int(&cant_lineas, mensaje);

                if(direccion_archivo==-10002){
                    direccion_archivo = -direccion_archivo;
                    abortar_dtb(plp, id_dtb, direccion_archivo, archivos_abiertos);
                }else {
                    desbloquear_dtb_cargando_archivo(pcp, id_dtb, path, direccion_archivo, cant_lineas);
                    dictionary_put(archivos_abiertos, path, NULL);
                }

                free(path);
                break;

            case SOLICITUD_RECURSO:
                recibir_int(&id_dtb, mensaje);
                recibir_string(&nombre_recurso, mensaje);
                copia_id_dtb = malloc(sizeof(int));
                *copia_id_dtb = id_dtb;

                if(!dictionary_has_key(pcp->recursos, nombre_recurso)){
                    recurso_seleccionado = malloc(sizeof(Recurso));
                    recurso_seleccionado->cola = queue_create();
                    queue_push(recurso_seleccionado->cola, copia_id_dtb);
                    recurso_seleccionado->disponibilidad = 0;
                    dictionary_put(pcp->recursos, nombre_recurso, recurso_seleccionado);
                    resultado = 1;
                }else{
                    recurso_seleccionado = dictionary_get(pcp->recursos, nombre_recurso);
                    if(recurso_seleccionado->disponibilidad > 0) {
                        recurso_seleccionado->disponibilidad--;
                        resultado = 1;
                        free(copia_id_dtb);
                    }else {
                        resultado = 0;
                        queue_push(recurso_seleccionado->cola, copia_id_dtb);
                    }
                }
                if(resultado)
                    log_info(logger, "Instancias de recurso '%s' decrementadas (total %d)", nombre_recurso,
                             recurso_seleccionado->disponibilidad);
                else
                    log_info(logger, "DTB %d intento solicitar recurso '%s' no disponible", id_dtb, nombre_recurso);

                mensaje_respuesta = crear_mensaje(SOLICITUD_RECURSO, mensaje->socket, 0);
                agregar_dato(mensaje_respuesta, sizeof(int), &resultado);
                enviar_mensaje(mensaje_respuesta);
                free(nombre_recurso);
                break;

            case LIBERAR_RECURSO:
                recibir_string(&nombre_recurso, mensaje);

                recurso_seleccionado = dictionary_get(pcp->recursos, nombre_recurso);

                if(recurso_seleccionado == NULL){
                    recurso_seleccionado = malloc(sizeof(Recurso));
                    recurso_seleccionado->cola = queue_create();
                    recurso_seleccionado->disponibilidad = 1;
                    dictionary_put(pcp->recursos, nombre_recurso, recurso_seleccionado);
                }else{
                    recurso_seleccionado->disponibilidad++;
                    if(!queue_is_empty(recurso_seleccionado->cola)){
                        copia_id_dtb = (int*)queue_pop(recurso_seleccionado->cola);
                        desbloquear_dtb(pcp, *copia_id_dtb);
                        free(copia_id_dtb);
                    }
                }
                log_info(logger, "Instancias de recurso '%s' incrementadas (total %d)", nombre_recurso,
                        recurso_seleccionado->disponibilidad);

                free(nombre_recurso);
                break;

            case CONSULTA_ARCHIVO_ABIERTO:
                recibir_string(&path, mensaje);

                if(dictionary_has_key(archivos_abiertos, path))
                    resultado = 0;
                else
                    resultado = 1;

                mensaje_respuesta = crear_mensaje(CONSULTA_ARCHIVO_ABIERTO, mensaje->socket, 0);
                agregar_dato(mensaje_respuesta, sizeof(int), &resultado);
                enviar_mensaje(mensaje_respuesta);
                free(path);
                break;

            case CERRAR_ARCHIVO_CPU_FM9:
                recibir_string(&path, mensaje);
                dictionary_remove(archivos_abiertos, path);
                free(path);
                break;

            case NUEVA_CONEXION:
                // se conecto un CPU

                if(mensaje->t_proceso == t_cpu) {
                    log_info(logger, "Aumentando cantidad de CPUs disponibles...");
                    sem_post(&cantidad_cpus);

                    identificador_cpu = list_size(conexiones_activas.lista_cpus)-1;
                    mensaje_respuesta = crear_mensaje(IDENTIFICADOR_CPU, mensaje->socket, 0);
                    agregar_dato(mensaje_respuesta, sizeof(int), &identificador_cpu);
                    enviar_mensaje(mensaje_respuesta);
                }

                if(conexiones_activas.procesos_conectados[t_elDiego] && conexiones_activas.procesos_conectados[t_cpu])
                    sem_post(&arrancar_planificadores);

                break;

            // en cada case del switch se puede manejar cada header como se desee
            case STRING_DIEGO_SAFA:
                // este header indica que el diego nos esta mandando un string

                // recibir_string recibe un stream de datos del socket del cual se envio el mensaje y los interpreta
                // como string, agregando \0 al final y metiendo los datos en el array str
                recibir_string(&str, mensaje);
                printf("s-AFA recibio: %s\n", str);
                free(str);

                // para probar la capacidad de comunicacion bidireccional, le contestamos un "hola!"
                // el header STRING_MDJ_DIEGO significa que le estamos mandando un string al diego desde MDJ
                mensaje_respuesta = crear_mensaje(STRING_SAFA_DIEGO, mensaje->socket, 0);
                agregar_string(mensaje_respuesta, "Hola!");
                enviar_mensaje(mensaje_respuesta);
                break;

            case CONEXION_CERRADA:
                // el header CONEXION_CERRADA indica que el que nos envio ese mensaje se desconecto, idealmente los
                // procesos que cierran deberian mandar este header antes de hacerlo para que los procesos a los cuales
                // estan conectados se enteren, de todas maneras esperar_mensaje se encarga internamente de cerrar
                // su socket, liberar memoria, etc

                if(mensaje->t_proceso == t_cpu) {
                    log_info(logger, "Reduciendo cantidad de CPUs disponibles...");
                    sem_wait(&cantidad_cpus);
                }
                break;

            default:
                log_error(logger, "Recibido header invalido (%d)", mensaje->header);
                break;
        }

        destruir_mensaje(mensaje);
    }
    return NULL;
}
