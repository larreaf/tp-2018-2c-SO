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
int socket_elDiego, socket_fm9, socket_safa, cantidad_instrucciones_ejecutadas, cantidad_instrucciones_dma;

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
	int conexiones_permitidas[cantidad_tipos_procesos] = {0};
    t_accion_post_instruccion resultado_instruccion;
	DTB datos_dtb;
	MensajeDinamico *peticion_abrir_script, *mensaje, *mensaje_respuesta;
    char* linea = "";

    remove("cpu.log");

    // TODO nombre CPU + numero de CPU
    logger = log_create("cpu.log", "cpu", true, log_level_from_string("info"));

    validar_parametros(argc);
    cfg_cpu* configuracion = asignar_config(argv[1],cpu);

    conexiones_activas = inicializar_conexiones_activas(logger, MY_IP,0, conexiones_permitidas, t_cpu);

	socket_fm9 = conectar_como_cliente(conexiones_activas, configuracion->ip_fm9, configuracion->puerto_fm9, t_fm9);
	if(socket_fm9!=-1)
	    log_info(logger, "Conexion con FM9 lista");
	else{
	    log_error(logger, "Conexion con FM9 denegada, cerrando CPU");
	    cerrar_cpu(logger, configuracion, conexiones_activas);
	}

    socket_safa = conectar_como_cliente(conexiones_activas, configuracion->ip_safa, configuracion->puerto_safa, t_safa);
    if(socket_safa!=-1)
        log_info(logger, "Conexion con SAFA lista");
    else{
        log_error(logger, "Conexion con SAFA denegada, cerrando CPU");
        cerrar_cpu(logger, configuracion, conexiones_activas);
    }

    socket_elDiego = conectar_como_cliente(conexiones_activas, configuracion->ip_elDiego, configuracion->puerto_elDiego,
            t_elDiego);
    if(socket_elDiego!=-1)
        log_info(logger, "Conexion con elDiego lista");
    else{
        log_error(logger, "Conexion con elDiego denegada, cerrando CPU");
        cerrar_cpu(logger, configuracion, conexiones_activas);
    }

    log_info(logger, "Listo");

	while(true){
	    mensaje = esperar_mensajes(conexiones_activas);

	    switch(mensaje->header){

	        case DATOS_DTB:
	            desempaquetar_dtb(mensaje, &datos_dtb);
                log_info(logger, "Datos DTB entrantes de DTB %d...", datos_dtb.id);
                resultado_instruccion = READY;
                cantidad_instrucciones_ejecutadas = 0;
                cantidad_instrucciones_dma = 0;

	            switch(datos_dtb.inicializado){
                    case 0:
                        // es el DTB dummy

                        log_info(logger, "Ejecutando DTB dummy, enviando peticion abrir %s", datos_dtb.path_script);
                        peticion_abrir_script = crear_mensaje(ABRIR_SCRIPT_CPU_DIEGO, socket_elDiego, 0);
                        agregar_dato(peticion_abrir_script, sizeof(int), &datos_dtb.id);
                        agregar_string(peticion_abrir_script, datos_dtb.path_script);
                        if(enviar_mensaje(peticion_abrir_script)==1)
                            log_info(logger, "Enviada peticion de abrir script");
                        else
                            log_error(logger, "Fallo al enviar peticion de abrir script");

                        resultado_instruccion = BLOQUEAR;
                        break;

	                case 1:
	                    // es un DTB comun

	                    while(datos_dtb.quantum && resultado_instruccion == READY) {

	                        // fetch de instruccion
                            mensaje_respuesta = crear_mensaje(LEER_LINEA, socket_fm9, 0);
                            agregar_dato(mensaje_respuesta, sizeof(int), &(datos_dtb.id));
                            agregar_dato(mensaje_respuesta, sizeof(int), &(datos_dtb.program_counter));
                            enviar_mensaje(mensaje_respuesta);

                            log_trace(logger, "Esperando respuesta de FM9...");

                            mensaje_respuesta = recibir_mensaje(socket_fm9);
                            recibir_string(&linea, mensaje_respuesta);
                            destruir_mensaje(mensaje_respuesta);

                            datos_dtb.program_counter++;

                            // los comentarios comienzan con #
                            if(string_starts_with(linea, "#"))
                                continue;
                            else if(string_starts_with(linea, "\n") || string_is_empty(linea)){
                                // se termino el programa
                                resultado_instruccion = DTB_EXIT;
                                break;
                            }

                            // ejecucion de instruccion
                            log_info(logger, "Quantum restante: %d | Ejecutando linea: %s", datos_dtb.quantum, linea);
                            resultado_instruccion = ejecutar_linea(&datos_dtb, linea,
                                    (unsigned int)configuracion->retardo);

                            datos_dtb.quantum--;

                            if(resultado_instruccion != -2)
                                cantidad_instrucciones_ejecutadas++;
                        }

                        if(resultado_instruccion == -1){
                            log_error(logger, "Cantidad de argumentos erroneos en DTB %d, linea: %s\n", datos_dtb.id,
                                    linea);
                        }
                        else if(resultado_instruccion == -2){
                            log_error(logger, "Operacion no reconocida en DTB %d, linea: %s\n", datos_dtb.id, linea);
                        }

                        break;

                    default:
                        log_error(logger, "Flag inicializado de DTB %d invalida, abortando DTB...", datos_dtb.id);
                        resultado_instruccion = -3;
                        break;
	            }
	            log_info(logger, "Volviendo DTB %d a SAFA con quantum restante %d y status %d", datos_dtb.id,
	                    datos_dtb.quantum, datos_dtb.status);
                datos_dtb.status = resultado_instruccion;

                if(datos_dtb.status != READY && datos_dtb.status != BLOQUEAR){
                    mensaje_respuesta = crear_mensaje(DESALOJAR_SCRIPT, socket_fm9, 0);
                    agregar_dato(mensaje_respuesta, sizeof(int), &datos_dtb.id);
                    enviar_mensaje(mensaje_respuesta);
                    // TODO flush archivos abiertos?
                }
                mensaje_respuesta = generar_mensaje_dtb(socket_safa, &datos_dtb);
                agregar_dato(mensaje_respuesta, sizeof(int), &cantidad_instrucciones_ejecutadas);
                agregar_dato(mensaje_respuesta, sizeof(int), &cantidad_instrucciones_dma);
                enviar_mensaje(mensaje_respuesta);
	            break;

            case CONEXION_CERRADA:
                switch(mensaje->t_proceso){
                    case t_safa:
                        log_error(logger, "CPU perdio conexion con SAFA, cerrando CPU");
                        cerrar_cpu(logger, configuracion, conexiones_activas);

                    case t_elDiego:
                        log_error(logger, "CPU perdio conexion con elDiego, cerrando CPU");
                        cerrar_cpu(logger, configuracion, conexiones_activas);

                    case t_fm9:
                        log_error(logger, "CPU perdio conexion con FM9, cerrando CPU");
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
