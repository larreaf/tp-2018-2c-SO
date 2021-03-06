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

int contar_lineas(char* string){
    int count = 0, len = strlen(string);

    for(int i = 0; i<len; i++)
        if(string[i] == '\n')
            count++;

    return count;
}

int main(int argc, char **argv) {
    int socket_mdj, socket_fm9, socket_safa, conexiones_permitidas[cantidad_tipos_procesos]={0};
    MensajeDinamico* nuevo_mensaje;
    ConexionesActivas conexiones_activas;
    int id_dtb, resultado, direccion_memoria, codigo_error, cant_lineas;
    char* path;
    char* archivo;
    MensajeDinamico* mensaje_dinamico;

	validar_parametros(argc);
	cfg_elDiego* configuracion = asignar_config(argv[1],elDiego);

	remove("elDiego.log");
	logger = log_create("elDiego.log", "elDiego", configuracion->logger_consola, log_level_from_string(configuracion->logger_level));

	conexiones_permitidas[t_cpu] = 10;
	conexiones_activas = inicializar_conexiones_activas(logger, configuracion->ip,configuracion->puerto, conexiones_permitidas, t_elDiego);

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
                destruir_mensaje(mensaje_dinamico);

                if(string_is_empty(archivo)) {
                    log_warning(logger, "Error al cargar script %s para DTB %d: no existe el script", path, id_dtb);
                    resultado = -4;
                    mensaje_dinamico = crear_mensaje(ABORTAR_DTB, socket_safa, 0);
                    agregar_int(mensaje_dinamico, id_dtb);
                    agregar_int(mensaje_dinamico, resultado);
                    enviar_mensaje(mensaje_dinamico);
                    break;
                }

                log_info(logger, "Recibido script %s de MDJ para DTB %d, enviando datos a FM9", path, id_dtb);

                // enviar mensaje a FM9 para cargar el script
                mensaje_dinamico = crear_mensaje(CARGAR_SCRIPT,socket_fm9, configuracion->transfer_size);
                agregar_int(mensaje_dinamico, id_dtb);
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
                    agregar_int(mensaje_dinamico, id_dtb);
                    enviar_mensaje(mensaje_dinamico);
                }else{
                    log_warning(logger, "Error al cargar script %s para DTB %d", path, id_dtb);

                    mensaje_dinamico = crear_mensaje(ABORTAR_DTB, socket_safa, 0);
                    agregar_int(mensaje_dinamico, id_dtb);
                    agregar_int(mensaje_dinamico, resultado);
                    enviar_mensaje(mensaje_dinamico);
                }
                free(archivo);
                free(path);
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

                if(cant_lineas>0) {
                    mensaje_dinamico = crear_mensaje(DESBLOQUEAR_DTB, socket_safa, 0);
                    agregar_int(mensaje_dinamico, id_dtb);
                }else{
                    log_warning(logger, "Error al crear archivo para DTB %d", id_dtb);
                    mensaje_dinamico = crear_mensaje(DESALOJAR_SCRIPT, socket_fm9, configuracion->transfer_size);
                    agregar_int(mensaje_dinamico, id_dtb);
                    enviar_mensaje(mensaje_dinamico);

                    mensaje_dinamico = crear_mensaje(ABORTAR_DTB, socket_safa, 0);
                    agregar_int(mensaje_dinamico, id_dtb);
                    agregar_int(mensaje_dinamico, 50002);
                }
                enviar_mensaje(mensaje_dinamico);
                free(path);
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
                destruir_mensaje(mensaje_dinamico);

                log_info(logger, "Recibido archivo %s de MDJ para DTB %d, enviando datos a FM9", path, id_dtb);

                // enviar mensaje a FM9 para cargar el archivo
                mensaje_dinamico = crear_mensaje(CARGAR_ARCHIVO,socket_fm9, configuracion->transfer_size);
                agregar_int(mensaje_dinamico, id_dtb);
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
                    cant_lineas = contar_lineas(archivo);
                }else{
                    log_warning(logger, "Error al abrir archivo %s para DTB %d", path, id_dtb);
                    cant_lineas = 0;
                }

                mensaje_dinamico = crear_mensaje(RESULTADO_CARGAR_ARCHIVO, socket_safa, 0);
                agregar_int(mensaje_dinamico, id_dtb);
                agregar_string(mensaje_dinamico, path);
                agregar_int(mensaje_dinamico, resultado);
                agregar_int(mensaje_dinamico, cant_lineas);
                enviar_mensaje(mensaje_dinamico);
                free(archivo);
                free(path);
                break;

            case BORRAR_ARCHIVO_CPU_DIEGO:
                recibir_int(&id_dtb, nuevo_mensaje);
                recibir_string(&path, nuevo_mensaje);

                //Enviar pedido a MDJ para borrar archivo
                mensaje_dinamico = crear_mensaje_mdj_borrar_archivo(socket_mdj, path, configuracion->transfer_size);
                log_info(logger, "Enviando mensaje a MDJ para borrar archivo %s", path);
                enviar_mensaje(mensaje_dinamico);

                // recibir respuesta de MDJ
                mensaje_dinamico = recibir_mensaje(socket_mdj);
                if(mensaje_dinamico->header != BORRAR_ARCHIVO){
                log_error(logger, "Falla al recibir respuesta de BORRAR ARCHIVO en MDJ");
                cerrar_elDiego(logger, configuracion, conexiones_activas);
                }

                recibir_int(&resultado, mensaje_dinamico);
                destruir_mensaje(mensaje_dinamico);

                //Segun el resultado de MDJ enviar a SAFA - Dsbloquear DTB o codigo de error
                if (!resultado){
                    	codigo_error=60001;
                    	log_error(logger, "Falla en BORRRAR ARCHIVO en mdj para id_dtb %d- Error %d",id_dtb,codigo_error);
                        mensaje_dinamico = crear_mensaje(ABORTAR_DTB, socket_safa, 0);
                        agregar_int(mensaje_dinamico, id_dtb);
                        agregar_int(mensaje_dinamico, codigo_error);
                        enviar_mensaje(mensaje_dinamico);
                        mensaje_dinamico = crear_mensaje(DESALOJAR_SCRIPT, socket_fm9, configuracion->transfer_size);
                        agregar_int(mensaje_dinamico, id_dtb);
                        enviar_mensaje(mensaje_dinamico);
                }else{
                        log_info(logger, "Se Borro el Archivo %s - BORRRAR ARCHIVO en mdj OK", path);
                        mensaje_dinamico = crear_mensaje(DESBLOQUEAR_DTB, socket_safa, 0);
                        agregar_int(mensaje_dinamico, id_dtb);
                        enviar_mensaje(mensaje_dinamico);
                }
            	break;

            case FLUSH_ARCHIVO:
                recibir_int(&id_dtb, nuevo_mensaje);
                recibir_int(&direccion_memoria, nuevo_mensaje);
                recibir_string(&path, nuevo_mensaje);

                //pedir archivo a FM9
                mensaje_dinamico = crear_mensaje(FLUSH_ARCHIVO, socket_fm9, configuracion->transfer_size);
                agregar_int(mensaje_dinamico, id_dtb);
                agregar_int(mensaje_dinamico, direccion_memoria);

                enviar_mensaje(mensaje_dinamico);

                // recibir respuesta de FM9
                mensaje_dinamico = recibir_mensaje(socket_fm9);
                if(mensaje_dinamico->header != RESULTADO_FLUSH_ARCHIVO){
                    log_error(logger, "Falla al recibir respuesta de obtener datos de FM9");
                    cerrar_elDiego(logger, configuracion, conexiones_activas);
                }

                recibir_string(&archivo, mensaje_dinamico);
                destruir_mensaje(mensaje_dinamico);

                if (string_is_empty(archivo)) {
                	codigo_error=40002;
                    log_error(logger, "Falla en flush archivo");
                    mensaje_dinamico = crear_mensaje(ABORTAR_DTB, socket_safa, 0);
                    agregar_int(mensaje_dinamico, id_dtb);
                    agregar_int(mensaje_dinamico, codigo_error);
                    enviar_mensaje(mensaje_dinamico);
                }else{
                	int size = strlen(archivo);

                	//guardar en mdj
                	mensaje_dinamico = crear_mensaje_mdj_guardar_datos(socket_mdj, path, 0, size, archivo, configuracion->transfer_size);
                	enviar_mensaje(mensaje_dinamico);
                	//recibir rta de mdj
                	mensaje_dinamico = recibir_mensaje(socket_mdj);
                	if(mensaje_dinamico->header != GUARDAR_DATOS){
                		log_error(logger, "Falla al recibir respuesta de obtener datos de FM9");
                        cerrar_elDiego(logger, configuracion, conexiones_activas);
                    }
                    recibir_int(&resultado, mensaje_dinamico);
                	destruir_mensaje(mensaje_dinamico);

                	size--;
                    if (resultado==-1){
                    	codigo_error=40001;
                        log_error(logger, "Falla en flush archivo guardar datos en mdj para id_dtb %d- Error %d",id_dtb,codigo_error);
                        mensaje_dinamico = crear_mensaje(ABORTAR_DTB, socket_safa, 0);
                        agregar_int(mensaje_dinamico, id_dtb);
                        agregar_int(mensaje_dinamico, codigo_error);
                        enviar_mensaje(mensaje_dinamico);
                        mensaje_dinamico = crear_mensaje(DESALOJAR_SCRIPT, socket_fm9, configuracion->transfer_size);
                        agregar_int(mensaje_dinamico, id_dtb);
                        enviar_mensaje(mensaje_dinamico);
                    }else{
                        log_info(logger, "Flush archivo guardar datos en mdj - OK");
                        mensaje_dinamico = crear_mensaje(DESBLOQUEAR_DTB, socket_safa, 0);
                        agregar_int(mensaje_dinamico, id_dtb);
                        enviar_mensaje(mensaje_dinamico);
                    }
                }
                free(path);
                free(archivo);

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
                break;

            case NUEVA_CONEXION:
                break;

            default:
                log_error(logger, "Se recibio un header invalido (%d)", nuevo_mensaje->header);
                break;
        }
        destruir_mensaje(nuevo_mensaje);
    }
}
