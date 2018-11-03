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

/*!
 * Ejecuta el servidor de safa para comunicarse con CPUs y elDiego
 * @param arg No utilizado
 * @return NULL cuando se recibe exit desde consola
 */
void* ejecutar_servidor(void *arg){
    int conexiones_permitidas[cantidad_tipos_procesos] = {0}, id_dtb, direccion_archivo, codigo_error;
    MensajeDinamico* mensaje_respuesta, *mensaje;
    char* str = NULL, *path;
    DTB datos_dtb;
    DTB* dtb_seleccionado;

    conexiones_permitidas[t_elDiego] = 1;
    conexiones_permitidas[t_consola_safa] = 1;
    conexiones_permitidas[t_cpu] = 1;
    conexiones_activas = inicializar_conexiones_activas(logger, configuracion->puerto, conexiones_permitidas, t_safa);

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

                if(datos_dtb.inicializado)
                    log_info(logger, "Recibidos datos de DTB %d", datos_dtb.id);
                else
                    log_info(logger, "Recibidos datos de DTB DUMMY");

                dtb_seleccionado = conseguir_y_actualizar_dtb(pcp, &datos_dtb);
                free(datos_dtb.path_script);
                decrementar_procesos_asignados_cpu(conexiones_activas, mensaje->socket);
                sem_post(&cantidad_cpus);

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
                        destruir_dtb(dtb_seleccionado);
                        sem_post(&plp->semaforo_multiprogramacion);
                        break;

                    default:
                        // los datos actualizados del DTB indican que produjo un error
                        log_error(logger, "El DTB %d (Path: %s) produjo un error (codigo %d), abortando",
                                datos_dtb.id, dtb_seleccionado->path_script, datos_dtb.status);
                        destruir_dtb(dtb_seleccionado);
                        sem_post(&plp->semaforo_multiprogramacion);
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

            case ABORTAR_DTB_DE_NEW:
                recibir_int(&id_dtb, mensaje);
                recibir_int(&codigo_error, mensaje);
                dtb_seleccionado = NULL;

                pthread_mutex_lock(&(plp->mutex_new));
                for(int i = 0; i<list_size(plp->lista_new); i++) {
                    dtb_seleccionado = list_get(plp->lista_new, i);

                    if (dtb_seleccionado->id == id_dtb){
                        list_remove(plp->lista_new, i);
                        break;
                    }
                }
                if(dtb_seleccionado == NULL) {
                    log_error(logger, "Error al abortar DTB %d de NEW", id_dtb);
                    break;
                }
                pthread_mutex_unlock(&(plp->mutex_new));

                log_error(logger, "El DTB %d (Path: %s) produjo un error (codigo %d), abortando", id_dtb,
                          dtb_seleccionado->path_script, abs(codigo_error));
                destruir_dtb(dtb_seleccionado);
                break;

            case ABORTAR_DTB:
                recibir_int(&id_dtb, mensaje);
                recibir_int(&codigo_error, mensaje);

                dtb_seleccionado = tomar_de_exec(pcp, id_dtb);
                if(dtb_seleccionado == NULL)
                    dtb_seleccionado = obtener_dtb_de_block(pcp, id_dtb);
                
                log_error(logger, "El DTB %d (Path: %s) produjo un error (codigo %d), abortando", id_dtb,
                        dtb_seleccionado->path_script, codigo_error);
                destruir_dtb(dtb_seleccionado);
                sem_post(&plp->semaforo_multiprogramacion);
                break;

            case RESULTADO_CARGAR_ARCHIVO:
                recibir_int(&id_dtb, mensaje);
                recibir_string(&path, mensaje);
                recibir_int(&direccion_archivo, mensaje);

                desbloquear_dtb_cargando_archivo(pcp, id_dtb, path, direccion_archivo);
                break;

            case NUEVA_CONEXION:
                // se conecto un CPU

                if(mensaje->t_proceso == t_cpu) {
                    log_info(logger, "Aumentando cantidad de CPUs disponibles...");
                    sem_post(&cantidad_cpus);
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
                // TODO aca habria que programar que pasa si se manda un header invalido

                break;
        }

        destruir_mensaje(mensaje);
    }
    return NULL;
}
