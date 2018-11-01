#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <string.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <commons/log.h>
#include <ensalada/validacion.h>
#include <ensalada/protocolo.h>
#include <ensalada/servidor.h>
#include <ensalada/mensaje.h>
#include <ensalada/com.h>
#include "mensajes_mdj.h"

t_log* logger;

void cerrar_elDiego(t_log* logger, cfg_elDiego* configuracion, ConexionesActivas conexiones_activas){
    log_info(logger, "Cerrando elDiego...");

    // destruir_conexiones_activas manda headers CONEXION_CERRADA a todos los clientes conectados para que se enteren y despues
    // cierra cada socket
    destruir_conexiones_activas(conexiones_activas);
    log_destroy(logger);
    destroy_cfg(configuracion, t_elDiego);
    exit(0);
}

int main(int argc, char **argv) {
    int socket_mdj, socket_fm9, socket_safa, conexiones_permitidas[cantidad_tipos_procesos]={0};
    MensajeDinamico* nuevo_mensaje;
    ConexionesActivas conexiones_activas;
    int id_dtb, resultado, cant_lineas;
    char* path;
    char* archivo;
    MensajeDinamico* mensaje_dinamico;

	validar_parametros(argc);
	cfg_elDiego* configuracion = asignar_config(argv[1],elDiego);

	logger = log_create("elDiego.log", "elDiego", true, log_level_from_string("info"));

	conexiones_permitidas[t_cpu] = 3;
	conexiones_activas = inicializar_conexiones_activas(logger, configuracion->puerto, conexiones_permitidas, t_elDiego);

	// conectar como cliente a MDJ
	socket_mdj = conectar_como_cliente(conexiones_activas, configuracion->ip_mdj, configuracion->puerto_mdj, t_mdj);

	// conectar como cliente a FM9
	socket_fm9 = conectar_como_cliente(conexiones_activas, configuracion->ip_fm9, configuracion->puerto_fm9, t_fm9);

    // conectar como cliente a SAFA
    socket_safa = conectar_como_cliente(conexiones_activas, configuracion->ip_safa, configuracion->puerto_safa, t_safa);

    log_info(logger, "Listo");

    while(1){
        nuevo_mensaje = esperar_mensajes(conexiones_activas);

        switch(nuevo_mensaje->header){
            case ABRIR_SCRIPT_CPU_DIEGO:
                // CPU pide abrir un script
                // aca habria que hacer un diccionario para asociar el path del script con el id del DTB que lo pidio

                recibir_int(&id_dtb, nuevo_mensaje);
                recibir_string(&path, nuevo_mensaje);

                log_info(logger, "Enviando mensaje para abrir script %s para el DTB %d (%d bytes)", path, id_dtb,
                         nuevo_mensaje->longitud);

                // enviar pedido a MDJ de abrir script
                mensaje_dinamico = crear_mensaje_mdj_obtener_datos(socket_mdj, path, 0, 0, configuracion->transfer_size);
                enviar_mensaje(mensaje_dinamico);

                // recibir respuesta de MDJ
                mensaje_dinamico = recibir_mensaje(socket_mdj);
                if(mensaje_dinamico->header != OBTENER_DATOS){
                    log_error(logger, "Falla al recibir respuesta de obtener datos de MDJ");
                    cerrar_elDiego(logger, configuracion, conexiones_activas);
                }
                recibir_string(&archivo, mensaje_dinamico);

                log_info(logger, "Recibido script %s de MDJ para DTB %d, enviando datos a FM9", path, id_dtb);
                printf("Contenido script:\n%s", archivo);

                // enviar mensaje a FM9 para cargar el script
                mensaje_dinamico = crear_mensaje(CARGAR_SCRIPT,socket_fm9, 64);
                agregar_dato(mensaje_dinamico, sizeof(int), &id_dtb);
                agregar_string(mensaje_dinamico, archivo);
                enviar_mensaje(mensaje_dinamico);

                // recibir respuesta de FM9
                log_info(logger, "Esperando respuesta de FM9...");
                mensaje_dinamico = recibir_mensaje(socket_fm9);
                if(mensaje_dinamico->header != RESULTADO_CARGAR_SCRIPT){
                    log_error(logger, "Falla al recibir respuesta de cargar script en FM9");
                    cerrar_elDiego(logger, configuracion, conexiones_activas);
                }

                recibir_int(&resultado, mensaje_dinamico);
                destruir_mensaje(mensaje_dinamico);

                // si da respuesta ok, notificar a safa para que desbloquee el dtb
                if(!resultado){
                    log_info(logger, "Script para el DTB %d abierto con exito, notificando a SAFA...", id_dtb);

                    mensaje_dinamico = crear_mensaje(PASAR_DTB_A_READY, socket_safa, 0);
                    agregar_dato(mensaje_dinamico, sizeof(int), &id_dtb);
                    enviar_mensaje(mensaje_dinamico);
                }else{
                    log_warning(logger, "Error al cargar script %s para DTB %d", path, id_dtb);
                    // TODO enviar error a SAFA para que aborte el DTB
                }

                break;

            case CREAR_ARCHIVO_CPU_DIEGO:
                recibir_int(&id_dtb, nuevo_mensaje);
                recibir_string(&path, nuevo_mensaje);
                recibir_int(&cant_lineas, nuevo_mensaje);

                log_info(logger, "Enviando mensaje a MDJ para crear archivo %s", path);
                mensaje_dinamico = crear_mensaje_mdj_crear_archivo(socket_mdj, path, cant_lineas,
                        configuracion->transfer_size);
                enviar_mensaje(mensaje_dinamico);

                log_info(logger, "Esperando respuesta de MDJ para crear archivo %s", path);
                mensaje_dinamico = recibir_mensaje(socket_mdj);
                if(mensaje_dinamico->header != CREAR_ARCHIVO){
                    log_error(logger, "Falla al recibir respuesta de crear archivo en MDJ");
                    cerrar_elDiego(logger, configuracion, conexiones_activas);
                }
                recibir_int(&cant_lineas, mensaje_dinamico);
                destruir_mensaje(mensaje_dinamico);

                if(cant_lineas) {
                    mensaje_dinamico = crear_mensaje(DESBLOQUEAR_DTB, socket_safa, 0);
                    agregar_dato(mensaje_dinamico, sizeof(int), &id_dtb);
                    enviar_mensaje(mensaje_dinamico);
                }else{
                    log_warning(logger, "Error al crear archivo para DTB %d", id_dtb);
                    // TODO enviar error a SAFA para que aborte el DTB
                }

                break;

            case ABRIR_ARCHIVO_CPU_DIEGO:
                recibir_int(&id_dtb, nuevo_mensaje);
                recibir_string(&path, nuevo_mensaje);

                // enviar pedido a MDJ de abrir archivo
                mensaje_dinamico = crear_mensaje_mdj_obtener_datos(socket_mdj, path, 0, 0, configuracion->transfer_size);
                enviar_mensaje(mensaje_dinamico);

                // recibir respuesta de MDJ
                mensaje_dinamico = recibir_mensaje(socket_mdj);
                if(mensaje_dinamico->header != OBTENER_DATOS){
                    log_error(logger, "Falla al recibir respuesta de obtener datos de MDJ");
                    cerrar_elDiego(logger, configuracion, conexiones_activas);
                }
                recibir_string(&archivo, mensaje_dinamico);

                log_info(logger, "Recibido archivo %s de MDJ para DTB %d, enviando datos a FM9", path, id_dtb);
                printf("Contenido archivo:\n%s", archivo);

                // enviar mensaje a FM9 para cargar el archivo
                mensaje_dinamico = crear_mensaje(CARGAR_ARCHIVO,socket_fm9, 64);
                agregar_dato(mensaje_dinamico, sizeof(int), &id_dtb);
                agregar_string(mensaje_dinamico, archivo);
                enviar_mensaje(mensaje_dinamico);

                // recibir respuesta de FM9
                log_info(logger, "Esperando respuesta de FM9...");
                mensaje_dinamico = recibir_mensaje(socket_fm9);
                if(mensaje_dinamico->header != RESULTADO_CARGAR_ARCHIVO){
                    log_error(logger, "Falla al recibir respuesta de cargar archivo en FM9");
                    cerrar_elDiego(logger, configuracion, conexiones_activas);
                }

                recibir_int(&resultado, mensaje_dinamico);
                destruir_mensaje(mensaje_dinamico);

                // si da respuesta ok, notificar a safa para que desbloquee el dtb y mandar direccion del archivo abierto
                if(resultado>=0){
                    log_info(logger, "Archivo para el DTB %d abierto con exito, notificando a SAFA...", id_dtb);
                }else{
                    log_warning(logger, "Error al abrir archivo %s para DTB %d", path, id_dtb);
                    // TODO enviar error a SAFA para que aborte el DTB
                }

                mensaje_dinamico = crear_mensaje(RESULTADO_CARGAR_ARCHIVO, socket_safa, 0);
                agregar_dato(mensaje_dinamico, sizeof(int), &id_dtb);
                agregar_string(mensaje_dinamico, path);
                agregar_dato(mensaje_dinamico, sizeof(int), &resultado);
                enviar_mensaje(mensaje_dinamico);

                break;

            case CONEXION_CERRADA:
                switch(nuevo_mensaje->t_proceso){
                    case t_safa:
                        log_error(logger, "elDiego perdio conexion con SAFA, cerrando elDiego");
                        cerrar_elDiego(logger, configuracion, conexiones_activas);

                    case t_fm9:
                        log_error(logger, "elDiego perdio conexion con FM9, cerrando elDiego");
                        cerrar_elDiego(logger, configuracion, conexiones_activas);

                    case t_mdj:
                        log_error(logger, "elDiego perdio conexion con MDJ, cerrando elDiego");
                        cerrar_elDiego(logger, configuracion, conexiones_activas);

                    default:
                        break;
                }

            default:
                break;
        }
        destruir_mensaje(nuevo_mensaje);
    }
}
