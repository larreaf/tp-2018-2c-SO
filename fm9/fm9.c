#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <sys/select.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>
#include <ensalada/com.h>
#include <ensalada/config-types.h>
#include <ensalada/mensaje.h>
#include <ensalada/protocolo.h>
#include <ensalada/servidor.h>
#include <ensalada/validacion.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "memoria.h"
#include "consola.h"

Memoria* memoria;
int correr = 1;
pthread_t thread_consola;

void cerrar_fm9(t_log* logger, cfg_fm9* config, ConexionesActivas conexiones_activas, Memoria* memoria){
    log_info(logger, "Cerrando FM9...");

    // destruir_conexiones_activas manda headers CONEXION_CERRADA a todos los clientes conectados para que se enteren y
    // despues cierra cada socket
    destruir_conexiones_activas(conexiones_activas);
    correr = 0;
    pthread_kill(thread_consola, SIGUSR1);
    pthread_join(thread_consola, NULL);
    log_destroy(logger);
    destroy_cfg(config, t_fm9);
    destruir_memoria(memoria);
    exit(0);
}

int main(int argc, char **argv) {

	int conexiones_permitidas[cantidad_tipos_procesos] = {0}, id_dtb, resultado, numero_linea, direccion;
    char* linea, *linea_leida;
    char* string_archivo;
    MensajeDinamico* mensaje, *mensaje_respuesta;
    ConexionesActivas conexiones_activas;
    MemoriaReal* storage;

    remove("fm9.log");



    validar_parametros(argc);
    cfg_fm9 *configuracion = asignar_config(argv[1], fm9);

    t_log *logger = log_create("fm9.log", "fm9", configuracion->logger_consola, log_level_from_string(configuracion->logger_level));
    log_info(logger, "Configuración cargada");
    log_info(logger, "Inicializando conexiones_activas...");
    conexiones_permitidas[t_cpu] = 10;
    conexiones_permitidas[t_elDiego] = 1;
    conexiones_activas = inicializar_conexiones_activas(logger, configuracion->ip,configuracion->puerto,
            conexiones_permitidas, t_fm9);

    storage = inicializar_memoria_real(configuracion->tamanio, configuracion->max_linea, configuracion->tam_pagina);
    memoria = inicializar_memoria(storage, configuracion->modo, configuracion->tam_max_segmento);

    pthread_create(&thread_consola, NULL, (void*)ejecutar_consola_fm9, NULL);

    log_info(logger, "Listo");

    while (1) {
        mensaje = esperar_mensajes(conexiones_activas);

        switch (mensaje->header) {

            case CARGAR_SCRIPT:
                recibir_int(&id_dtb, mensaje);
                recibir_string(&string_archivo, mensaje);

                resultado = cargar_script(memoria, id_dtb, string_archivo);
                free(string_archivo);

                if (!resultado)
                    log_info(logger, "Script cargado en memoria correctamente, enviando mensaje respuesta");
                else
                    log_warning(logger, "Falla al cargar script en memoria (codigo error %d)", abs(resultado));

                mensaje_respuesta = crear_mensaje(RESULTADO_CARGAR_SCRIPT, mensaje->socket, mensaje->particionado);
                agregar_int(mensaje_respuesta, resultado);
                if(enviar_mensaje(mensaje_respuesta)==1)
                    log_trace(logger, "Mensaje respuesta enviado correctamente");

                break;

            case LEER_LINEA:
                recibir_int(&id_dtb, mensaje);
                recibir_int(&numero_linea, mensaje);

                linea = string_new();
                linea_leida = leer_linea(memoria, id_dtb, numero_linea);
                string_append(&linea, linea_leida);

                mensaje_respuesta = crear_mensaje(RESULTADO_LEER_LINEA, mensaje->socket, mensaje->particionado);
                agregar_string(mensaje_respuesta, linea);
                if(enviar_mensaje(mensaje_respuesta)==1)
                    log_trace(logger, "Mensaje respuesta enviado correctamente");

                free(linea);
                free(linea_leida);
                break;

            case CARGAR_ARCHIVO:
                recibir_int(&id_dtb, mensaje);
                recibir_string(&string_archivo, mensaje);

                resultado = cargar_archivo(memoria, id_dtb, string_archivo);
                free(string_archivo);

                if (resultado >= 0)
                    log_info(logger, "Archivo cargado en memoria correctamente, enviando mensaje respuesta");
                else {
                    log_warning(logger, "Falla al cargar archivo en memoria (codigo error %d)", abs(resultado));
                    desalojar_script(memoria, id_dtb);
                }

                mensaje_respuesta = crear_mensaje(RESULTADO_CARGAR_ARCHIVO, mensaje->socket, mensaje->particionado);
                agregar_int(mensaje_respuesta, resultado);
                enviar_mensaje(mensaje_respuesta);
                log_trace(logger, "Mensaje respuesta enviado correctamente");

                break;

            case ASIGNAR_ARCHIVO_CPU_FM9:
                recibir_int(&id_dtb, mensaje);
                recibir_int(&direccion, mensaje);
                recibir_string(&linea, mensaje);

                resultado = modificar_linea_archivo(memoria, id_dtb, direccion, linea);

                if (!resultado)
                    log_info(logger, "Archivo modificado exitosamente, enviando mensaje respuesta");
                else {
                    log_warning(logger, "Falla al modificar archivo en memoria (codigo error %d)", resultado);
                    desalojar_script(memoria, id_dtb);
                }

                mensaje_respuesta = crear_mensaje(ASIGNAR_ARCHIVO_CPU_FM9, mensaje->socket, 0);
                agregar_int(mensaje_respuesta, resultado);
                enviar_mensaje(mensaje_respuesta);

                break;

            case FLUSH_ARCHIVO:
                recibir_int(&id_dtb, mensaje);
                recibir_int(&direccion, mensaje);

                string_archivo = string_new();
                string_append(&string_archivo, flush_archivo(memoria, id_dtb, direccion));

                if (string_is_empty(string_archivo)) {
                    log_warning(memoria->logger, "Falla en flush archivo");
                    desalojar_script(memoria, id_dtb);
                }

                mensaje_respuesta = crear_mensaje(RESULTADO_FLUSH_ARCHIVO, mensaje->socket, mensaje->particionado);
                agregar_string(mensaje_respuesta, string_archivo);

                if(enviar_mensaje(mensaje_respuesta)==1)
                    log_trace(logger, "Mensaje respuesta enviado correctamente");

                break;

            case CERRAR_ARCHIVO_CPU_FM9:
                recibir_int(&id_dtb, mensaje);
                recibir_int(&direccion, mensaje);

                resultado = cerrar_archivo(memoria, id_dtb, direccion);

                if (!resultado)
                    log_info(memoria->logger, "Archivo en direccion %d liberado con exito", direccion);
                else {
                    log_warning(memoria->logger, "Falla al cerrar archivo en direccion %d", direccion);
                    desalojar_script(memoria, id_dtb);
                }

                break;

            case DESALOJAR_SCRIPT:
                recibir_int(&id_dtb, mensaje);

                resultado = desalojar_script(memoria, id_dtb);

                if(!resultado)
                    log_info(memoria->logger, "Script de DTB %d desalojado con exito", id_dtb);
                else
                    log_warning(memoria->logger, "Falla al desalojar script de DTB %d", id_dtb);

                break;

            case CONEXION_CERRADA:
                if (mensaje->t_proceso == t_elDiego)
                    cerrar_fm9(logger, configuracion, conexiones_activas, memoria);
                break;

            case NUEVA_CONEXION:
                break;

            default:
                log_error(logger, "Se recibio un header invalido (%d)", mensaje->header);
                break;
        }
    }
}
