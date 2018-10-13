#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <string.h>
#include <commons/log.h>
#include <ensalada/validacion.h>
#include <ensalada/protocolo.h>
#include <ensalada/servidor.h>
#include <ensalada/mensaje.h>
#include <ensalada/com.h>

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
    int id_dtb;
    char* path;

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
                recibir_int(&id_dtb, nuevo_mensaje);
                recibir_string(&path, nuevo_mensaje);

                printf("Pedido abrir script de DTB %d\n", id_dtb);
                printf("Path %s\n", path);
                break;

            case CONEXION_CERRADA:
                switch(nuevo_mensaje->t_proceso){
                    case t_safa:
                        log_warning(logger, "elDiego perdio conexion con SAFA, cerrando elDiego");
                        cerrar_elDiego(logger, configuracion, conexiones_activas);

                    case t_fm9:
                        log_warning(logger, "elDiego perdio conexion con FM9, cerrando elDiego");
                        cerrar_elDiego(logger, configuracion, conexiones_activas);

                    case t_mdj:
                        log_warning(logger, "elDiego perdio conexion con MDJ, cerrando elDiego");
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
