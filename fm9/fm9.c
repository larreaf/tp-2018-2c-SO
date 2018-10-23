#include <funciones.h>

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

DiegoAFM9* datos_fm9;

int main(int argc, char **argv) {
	int conexiones_permitidas[cantidad_tipos_procesos] = {0};
    char* str;
    MensajeDinamico* mensaje;
    ConexionesActivas conexiones_activas;

    t_log *logger = log_create("fm9.log", "fm9", true, log_level_from_string("info"));

    log_info(logger, "Inicializando config...");
    validar_parametros(argc);
    cfg_fm9 *configuracion = asignar_config(argv[1], fm9);

    log_info(logger, "Inicializando conexiones_activas...");
    conexiones_permitidas[t_cpu] = 1;
    conexiones_permitidas[elDiego] = 1;
    conexiones_activas = inicializar_conexiones_activas(logger, configuracion->puerto, conexiones_permitidas, t_fm9);

    inicializar_storage(logger, configuracion->tamanio);
    elegir_tipo_funcionamiento_memoria(logger, configuracion->modo);

    log_info(logger, "Listo");

    while (1){
        mensaje = esperar_mensajes(conexiones_activas);

        switch (mensaje->header) {
            case STRING_DIEGO_FM9:

            	desempaquetar_mensaje_diego_a_fm9(logger,mensaje, datos_fm9);
            	log_info(logger, "Datos entrantes de Diego a fm9 %d...", datos_fm9->id);

				switch(datos_fm9->instruccion){

					case "leer":
						log_info(logger, "Buscando datos solicitados por el Diego...", datos_fm9->id);
						return buscar_en_memoria(logger, datos_fm9);
					break;

					case "escribir":
						log_info(logger, "Escribiendo datos solicitados por el Diego...", datos_fm9->id);
						return escribir_en_memoria(logger, datos_fm9);
					break;

					case "modificar":
						log_info(logger, "Modificando datos solicitados por el Diego...", datos_fm9->id);
						return modificar_en_memoria(logger, datos_fm9);
					break;

					default:
						log_info(logger, "Se desconoce esta instrucción...", datos_fm9->instruccion);
					break;
				}
            	//str = recibir_string(mensaje->socket);
                //printf("Recibio: %s\n", str);

                //hay que cambiar esto ya que de el diego puede recibir porcion de codigo o puntero a espacio de memoria

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
        destruir_mensaje(mensaje);
    }
}
