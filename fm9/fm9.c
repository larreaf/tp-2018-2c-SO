#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <sys/select.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <ensalada/com.h>
#include <ensalada/config-types.h>
#include <ensalada/mensaje.h>
#include <ensalada/protocolo.h>
#include <ensalada/servidor.h>
#include <ensalada/validacion.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "memoria.h"

void cerrar_fm9(t_log* logger, cfg_fm9* config, ConexionesActivas conexiones_activas){
    log_info(logger, "Cerrando FM9...");

    // destruir_conexiones_activas manda headers CONEXION_CERRADA a todos los clientes conectados para que se enteren y despues
    // cierra cada socket
    destruir_conexiones_activas(conexiones_activas);
    log_destroy(logger);
    destroy_cfg(config, t_fm9);
    exit(0);
}


int main(int argc, char **argv) {
	int conexiones_permitidas[cantidad_tipos_procesos] = {0}, id_dtb, resultado, numero_linea, direccion;
    char* str, *linea;
    char* string_archivo;
    MensajeDinamico* mensaje, *mensaje_respuesta;
    ConexionesActivas conexiones_activas;
    MemoriaReal* storage;
    Memoria* memoria;

    t_log *logger = log_create("fm9.log", "fm9", true, log_level_from_string("info"));

    log_info(logger, "Inicializando config...");
    validar_parametros(argc);
    cfg_fm9 *configuracion = asignar_config(argv[1], fm9);

    log_info(logger, "Inicializando conexiones_activas...");
    conexiones_permitidas[t_cpu] = 1;
    conexiones_permitidas[elDiego] = 1;
    conexiones_activas = inicializar_conexiones_activas(logger, configuracion->puerto, conexiones_permitidas, t_fm9);

    storage = inicializar_memoria_real(configuracion->tamanio, configuracion->max_linea);
    memoria = inicializar_memoria(storage, configuracion->modo);

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
                    log_error(logger, "Falla al cargar script en memoria (codigo error %d)", resultado);

                mensaje_respuesta = crear_mensaje(RESULTADO_CARGAR_SCRIPT, mensaje->socket, 0);
                agregar_dato(mensaje_respuesta, sizeof(int), &resultado);
                enviar_mensaje(mensaje_respuesta);
                log_trace(logger, "Mensaje respuesta enviado correctamente");

                break;

            case LEER_LINEA:
                recibir_int(&id_dtb, mensaje);
                recibir_int(&numero_linea, mensaje);

                linea = string_new();
                string_append(&linea, leer_linea(memoria, id_dtb, numero_linea));

                mensaje_respuesta = crear_mensaje(RESULTADO_LEER_LINEA, mensaje->socket, 0);
                agregar_string(mensaje_respuesta, linea);
                enviar_mensaje(mensaje_respuesta);

                break;

            case CARGAR_ARCHIVO:
                recibir_int(&id_dtb, mensaje);
                recibir_string(&string_archivo, mensaje);

                resultado = cargar_archivo(memoria, id_dtb, string_archivo);
                free(string_archivo);

                if (resultado >= 0)
                    log_info(logger, "Archivo cargado en memoria correctamente, enviando mensaje respuesta");
                else
                    log_error(logger, "Falla al cargar archivo en memoria (codigo error %d)", resultado);

                mensaje_respuesta = crear_mensaje(RESULTADO_CARGAR_ARCHIVO, mensaje->socket, 0);
                agregar_dato(mensaje_respuesta, sizeof(int), &resultado);
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
                else
                    log_error(logger, "Falla al modificar archivo en memoria (codigo error %d)", resultado);

                break;

            case FLUSH_ARCHIVO:
                recibir_int(&id_dtb, mensaje);
                recibir_int(&direccion, mensaje);

                string_archivo = string_new();
                string_append(&string_archivo, flush_archivo(memoria, id_dtb, direccion));

                if (string_is_empty(string_archivo)) {
                    log_error(memoria->logger, "Falla en flush archivo");
                }

                printf("%s\n", string_archivo);

                break;

            case CERRAR_ARCHIVO_CPU_FM9:
                recibir_int(&id_dtb, mensaje);
                recibir_int(&direccion, mensaje);

                resultado = cerrar_archivo(memoria, id_dtb, direccion);

                if (!resultado)
                    log_info(memoria->logger, "Archivo en direccion %d liberado con exito", direccion);
                else
                    log_error(memoria->logger, "Falla al cerrar archivo en direccion %d", direccion);

                break;

            case CONEXION_CERRADA:
                if (mensaje->t_proceso == t_elDiego)
                    cerrar_fm9(logger, configuracion, conexiones_activas);
                break;

            default:
                break;
        }
    }
}
