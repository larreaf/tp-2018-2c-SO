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

void* puntero = NULL;
void* comienzo_storage = NULL;
void* datos_fm9 = NULL;

//todo crear archivo .h y .c de funciones fm9
typedef struct{
    int id;
    char* instruccion;
    char* contenido;
}DiegoAFM9;

void desempaquetar_mensaje_diego_a_fm9(MensajeDinamico* mensaje, DiegoAFM9* DiegoAFM9){

    recibir_int(&DiegoAFM9->id, mensaje);
    recibir_string(&DiegoAFM9->instruccion, mensaje);
    recibir_string(&DiegoAFM9->contenido, mensaje);
}

void destruir_storage(){
	free(comienzo_storage);
	free(puntero);
}

void* asignar_memoria(t_log *logger, int tamanioMemoria){
	//esta funcion deberia estar en ensalada quiza
	puntero = malloc(tamanioMemoria);

	if(puntero == NULL)
	{
		log_info(logger, "No se puede asignar ese espacio de memoria...");
	}
	else
	{
		log_info(logger, "Se asigno correctamente el espacio de memoria necesario...");
	}

	return puntero;
}

void inicializar_storage(t_log *logger,int tamanioMemoria){

	log_info(logger, "Inicializando storage según archivo de configuración...");

	comienzo_storage = asignar_memoria(logger, tamanioMemoria);
	//que hace si asigna_memoria devuelve 0 (no pudo asignar)
	// para mi tendriamos que en este caso cerrar fm9 o volver a solicitarlo cada tanto tiempo con un tope de veces

}

void cerrar_fm9(t_log* logger, cfg_fm9* configuracion, ConexionesActivas server){
    log_info(logger, "Cerrando FM9...");

    // destruir_conexiones_activas manda headers CONEXION_CERRADA a todos los clientes conectados para que se enteren y despues
    // cierra cada socket
    destruir_conexiones_activas(server);
    log_destroy(logger);
    destroy_cfg(configuracion, t_fm9);
    destruir_storage();
    exit(0);
}

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

    log_info(logger, "Listo");

    while (1){
        mensaje = esperar_mensajes(conexiones_activas);

        switch (mensaje->header) {
            case STRING_DIEGO_FM9:
            	//todo esta funcion datos_fm9->instruccion,datos_fm9->id
            	desempaquetar_mensaje_diego_a_fm9(mensaje, &datos_fm9);
            	log_info(logger, "Datos entrantes de Diego a fm9 %d...", datos_fm9.id);

				switch(datos_fm9.instruccion){

					case leer:
					break;

					case escribir:
					break;

					case modificar:
					break;

					default:
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
