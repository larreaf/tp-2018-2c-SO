#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <string.h>
#include <commons/log.h>
#include <ensalada/protocolo.h>
#include <ensalada/validacion.h>
#include <ensalada/com.h>
#include <ensalada/mensaje.h>
#include <ensalada/servidor.h>
#include <ensalada/dtb.h>
#include "parser.h"

ConexionesActivas conexiones_activas;
t_log* logger;

void cerrar_cpu(t_log* logger, cfg_cpu* configuracion, ConexionesActivas conexiones_activas){
    log_info(logger, "Cerrando CPU...");

    // destruir_conexiones_activas manda headers CONEXION_CERRADA a todos los clientes conectados para que se enteren y despues
    // cierra cada socket
    destruir_conexiones_activas(conexiones_activas);
    log_destroy(logger);
    destroy_cfg(configuracion, t_cpu);
    exit(0);
}

int main(int argc, char **argv) {
	int socket_fm9, socket_safa, socket_elDiego;
	int conexiones_permitidas[cantidad_tipos_procesos] = {0};
    t_accion_post_instruccion resultado_instruccion = READY;
	DTB datos_dtb;
	MensajeDinamico *peticion_abrir_script, *mensaje;
    char* linea = "";

    logger = log_create("cpu.log", "cpu", true, log_level_from_string("info"));

    validar_parametros(argc);
    cfg_cpu* configuracion = asignar_config(argv[1],cpu);

    conexiones_activas = inicializar_conexiones_activas(logger, 0, conexiones_permitidas, t_cpu);

	socket_fm9 = conectar_como_cliente(conexiones_activas, configuracion->ip_fm9, configuracion->puerto_fm9, t_fm9);
	log_info(logger, "Conexion con FM9 lista");

    socket_safa = conectar_como_cliente(conexiones_activas, configuracion->ip_safa, configuracion->puerto_safa, t_safa);
    log_info(logger, "Conexion con s-AFA lista");

    socket_elDiego = conectar_como_cliente(conexiones_activas, configuracion->ip_elDiego, configuracion->puerto_elDiego,
            t_elDiego);
    log_info(logger, "Conexion con elDiego lista");

    log_info(logger, "Listo");

	while(true){
	    mensaje = esperar_mensajes(conexiones_activas);

	    switch(mensaje->header){

	        case DATOS_DTB:
	            desempaquetar_dtb(mensaje, &datos_dtb);
                log_info(logger, "Datos DTB entrantes de DTB %d...", datos_dtb.id);

	            switch(datos_dtb.inicializado){
                    case 0:
                        // es el DTB dummy

                        log_info(logger, "Ejecutando DTB dummy, enviando peticion abrir %s", datos_dtb.path_script);
                        peticion_abrir_script = crear_mensaje(ABRIR_SCRIPT_CPU_DIEGO, socket_elDiego);
                        agregar_dato(peticion_abrir_script, sizeof(int), &datos_dtb.id);
                        agregar_string(peticion_abrir_script, datos_dtb.path_script);
                        enviar_mensaje(peticion_abrir_script);
                        datos_dtb.status = BLOQUEAR;
                        enviar_datos_dtb(socket_safa, &datos_dtb);

                        resultado_instruccion = BLOQUEAR;
                        break;

	                case 1:
	                    // es un DTB comun

	                    while(resultado_instruccion == READY) {
                            // TODO ir a buscar la linea indicada por el program counter y ejecutarla
                            linea = "";

                            // los comentarios comienzan con #
                            if(string_starts_with(linea, "#"))
                                continue;

                            resultado_instruccion = ejecutar_linea(&datos_dtb, linea,
                                    (unsigned int) configuracion->retardo);

                            datos_dtb.program_counter++;
                        }

                        if(resultado_instruccion == -1){
                            log_error(logger, "Cantidad de argumentos erroneos en DTB %d, linea: %s\n", datos_dtb.id,
                                    linea);
                            
                            resultado_instruccion = DTB_EXIT;
                        }
                        else if(resultado_instruccion == -2){
                            log_error(logger, "Operacion no reconocida en DTB %d, linea: %s\n", datos_dtb.id,
                                      linea);

                            resultado_instruccion = DTB_EXIT;
                        }
                            
	                    datos_dtb.status = resultado_instruccion;
	                    enviar_datos_dtb(socket_safa, &datos_dtb);
                        break;

                    default:
                        log_error(logger, "Flag inicializado de DTB %d invalida, abortando DTB...", datos_dtb.id);
                        resultado_instruccion = DTB_EXIT;
                        break;
	            }

	            break;

            case CONEXION_CERRADA:
                switch(mensaje->t_proceso){
                    case t_safa:
                        log_warning(logger, "CPU perdio conexion con SAFA, cerrando CPU");
                        cerrar_cpu(logger, configuracion, conexiones_activas);

                    case t_elDiego:
                        log_warning(logger, "CPU perdio conexion con elDiego, cerrando CPU");
                        cerrar_cpu(logger, configuracion, conexiones_activas);

                    case t_fm9:
                        log_warning(logger, "CPU perdio conexion con FM9, cerrando CPU");
                        cerrar_cpu(logger, configuracion, conexiones_activas);

                    default:
                        break;
                }

            default:
                // header invalido
                break;
	    }

	    destruir_mensaje(mensaje);
	}
}
