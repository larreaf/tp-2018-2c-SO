//
// Created by utnso on 25/09/18.
//

#include "servidor_safa.h"

extern ConexionesActivas conexiones_activas;
extern t_log* logger;
extern cfg_safa* configuracion;
extern PCP* pcp;
extern sem_t cantidad_cpus;

/*!
 * Ejecuta el servidor de safa para comunicarse con CPUs y elDiego
 * @param arg No utilizado
 * @return No retorna
 */
void* ejecutar_servidor(void *arg){
    int conexiones_permitidas[cantidad_tipos_procesos] = {0};
    MensajeDinamico* mensaje_respuesta, *mensaje;
    char* str = NULL;
    DTB datos_dtb;
    DTB* dtb_seleccionado;

    conexiones_permitidas[t_elDiego] = 1;
    conexiones_permitidas[t_consola_safa] = 1;
    conexiones_permitidas[t_cpu] = 1;
    conexiones_activas = inicializar_conexiones_activas(logger, configuracion->puerto, conexiones_permitidas, t_safa);

    while (1) {

        // bloquea hasta recibir un MensajeEntrante y lo retorna, ademas internamente maneja handshakes y desconexiones
        // sin retornar
        mensaje = esperar_mensajes(conexiones_activas);

        // cuando esperar_mensajes retorna, devuelve un MensajeEntrante que tiene como campos el socket que lo envio,
        // el header que se envio y el tipo de proceso que lo envio
        switch (mensaje->header) {
            case DATOS_DTB:
                // un CPU termino de ejecutar instrucciones y manda los datos del DTB actualizados

                desempaquetar_dtb(mensaje, &datos_dtb);
                log_info(logger, "Recibidos datos de DTB %d", datos_dtb.id);
                dtb_seleccionado = conseguir_y_actualizar_dtb(pcp, &datos_dtb);
                free(datos_dtb.path_script);
                decrementar_procesos_asignados_cpu(conexiones_activas, mensaje->socket);
                sem_post(&cantidad_cpus);

                switch(datos_dtb.status){

                    case BLOQUEAR:
                        // los datos actualizados del DTB indican que debe ser bloqueado

                        agregar_a_block(pcp, dtb_seleccionado);

                    case DTB_EXIT:
                        // los datos actualizados del DTB indican que debe ser pasado a exit

                        // TODO destruir DTB
                        break;

                    default:
                        // los datos actualizados del DTB indican que produjo un error
                        // TODO abortar el DTB/GDT

                        log_error(logger, "El DTB  %d (Path: %s) produjo un error (codigo %d), abortando",
                                datos_dtb.id, datos_dtb.path_script, datos_dtb.status);
                        break;
                }
                break;

            case NUEVA_CONEXION_CPU:
                // se conecto un CPU

                sem_post(&cantidad_cpus);
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
                mensaje_respuesta = crear_mensaje(STRING_SAFA_DIEGO, mensaje->socket);
                agregar_string(mensaje_respuesta, "Hola!");
                enviar_mensaje(mensaje_respuesta);
                break;

            case CONEXION_CERRADA:
                // el header CONEXION_CERRADA indica que el que nos envio ese mensaje se desconecto, idealmente los
                // procesos que cierran deberian mandar este header antes de hacerlo para que los procesos a los cuales
                // estan conectados se enteren, de todas maneras esperar_mensaje se encarga internamente de cerrar
                // su socket, liberar memoria, etc
                break;

            default:
                // TODO aca habria que programar que pasa si se manda un header invalido

                break;
        }

        destruir_mensaje(mensaje);
    }

}
