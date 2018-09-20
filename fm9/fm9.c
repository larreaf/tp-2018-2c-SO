#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <sys/select.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <ensalada/com.h>
#include <ensalada/protocolo.h>
#include <ensalada/servidor.h>
#include <ensalada/validacion.h>

void cerrar_fm9(t_log* logger, cfg_fm9* configuracion, ConexionesActivas server){
    log_info(logger, "Cerrando FM9...");

    // destruir_conexiones_activas manda headers CONEXION_CERRADA a todos los clientes conectados para que se enteren y despues
    // cierra cada socket
    destruir_conexiones_activas(server);
    log_destroy(logger);
    destroy_cfg(configuracion, t_fm9);
    exit(0);
}

int main(int argc, char **argv) {
    int conexiones_permitidas[cantidad_tipos_procesos] = {0};
    char* str;
    MensajeEntrante mensaje;
    ConexionesActivas conexiones_activas;

    t_log *logger = log_create("fm9.log", "fm9", true, log_level_from_string("info"));

    log_info(logger, "Inicializando config...");
    validar_parametros(argc);
    cfg_fm9 *configuracion = asignar_config(argv[1], fm9);

    log_info(logger, "Inicializando conexiones_activas...");
    conexiones_permitidas[t_cpu] = 1;
    conexiones_permitidas[elDiego] = 1;
    conexiones_activas = inicializar_conexiones_activas(logger, configuracion->puerto, conexiones_permitidas, t_fm9);

    log_info(logger, "Listo");

    while (1){
        mensaje = esperar_mensajes(conexiones_activas);

        switch (mensaje.header) {
            case STRING_DIEGO_FM9:
                str = recibir_string(mensaje.socket);
                printf("Recibio: %s\n", str);

                if(!strcmp(str, "exit"))
                    cerrar_fm9(logger, configuracion, conexiones_activas);

                break;

            case CONEXION_CERRADA:
                // ejemplo si se cierra elDiego y nos manda un header CONEXION_CERRADA, fm9 se cierra tambien
                /*
                if(mensaje.t_proceso==t_elDiego)
                    cerrar_fm9(logger, configuracion, conexiones_activas);

                */
                break;
            default:
                break;

        }
    }
}
