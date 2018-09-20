#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <string.h>
#include <commons/log.h>
#include <ensalada/protocolo.h>
#include <ensalada/validacion.h>
#include <ensalada/com.h>
#include <ensalada/servidor.h>

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
	int socket_fm9, socket_safa, socket_elDiego, header;
	int conexiones_permitidas[cantidad_tipos_procesos] = {0};
	ConexionesActivas conexiones_activas;
	MensajeEntrante mensaje;
    t_log* logger;

    logger = log_create("cpu.log", "cpu", true, log_level_from_string("info"));

    validar_parametros(argc);
    cfg_cpu* configuracion = asignar_config(argv[1],cpu);

    conexiones_activas = inicializar_conexiones_activas(logger, 0, conexiones_permitidas, t_cpu);

	socket_fm9 = conectar_como_cliente(conexiones_activas, configuracion->ip_fm9, configuracion->puerto_fm9, t_fm9);
	log_info(logger, "Conexion con FM9 lista");

    socket_safa = conectar_como_cliente(conexiones_activas, configuracion->ip_safa, configuracion->puerto_safa, t_safa);
    log_info(logger, "Conexion con s-AFA lista");

    socket_elDiego = conectar_como_cliente(conexiones_activas, configuracion->ip_elDiego, configuracion->puerto_elDiego, t_elDiego);
    log_info(logger, "Conexion con elDiego lista");

    log_info(logger, "Listo");

	while(true){
	    mensaje = esperar_mensajes(conexiones_activas);

	    switch(mensaje.header){
            case CONEXION_CERRADA:
                switch(mensaje.t_proceso){
                    case t_safa:
                        log_warning(logger, "CPU perdio conexion con SAFA, cerrando CPU");
                        cerrar_cpu(logger, configuracion, conexiones_activas);

                    case t_elDiego:
                        log_warning(logger, "CPU perdio conexion con elDiego, cerrando CPU");
                        cerrar_cpu(logger, configuracion, conexiones_activas);

                    default:
                        break;
                }
	    }
	}
}
