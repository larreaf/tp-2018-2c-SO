/*
 ============================================================================
 Name        : mdj.c
 Author      : 
 Version     :
 Copyright   : 
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include "validacion.h"
#include <netinet/in.h>
#include <commons/string.h>
#include <commons/log.h>
#include "protocolo.h"
#include "servidor.h"

void cerrar_mdj(t_log* logger, cfg_mdj* configuracion, Servidor server){
    log_info(logger, "Cerrando MDJ...");

    // destruir_servidor manda headers CONEXION_CERRADA a todos los clientes conectados para que se enteren y despues
    // cierra cada socket
    destruir_servidor(server);
    log_destroy(logger);
    destroy_cfg(configuracion, t_mdj);
    exit(0);
}

int main(int argc, char **argv) {
	char str[TAMANIO_MAXIMO_STRING];
	int conexiones_permitidas[4]={0};
	t_log* logger;
	Servidor server;
	MensajeEntrante mensaje;

    logger = log_create("mdj.log", "mdj", true, log_level_from_string("info"));
	validar_parametros(argc);
	cfg_mdj* configuracion = asignar_config(argv[1],mdj);

	// conexiones_permitidas es un array de 4 ints que indica que procesos se pueden conectar, o en el caso de t_cpu,
	// cuantas conexiones de cpu se van a aceptar
	conexiones_permitidas[t_elDiego] = 1;
	server = inicializar_servidor(logger, configuracion->puerto, conexiones_permitidas);

    while (1){
        // bloquea hasta recibir un MensajeEntrante y lo retorna, ademas internamente maneja handshakes y desconexiones
        // sin retornar
        mensaje = esperar_mensajes(server);

        // cuando esperar_mensajes retorna, devuelve un MensajeEntrante que tiene como campos el socket que lo envio,
        // el header que se envio y el tipo de proceso que lo envio
        switch (mensaje.header) {

            // en cada case del switch se puede manejar cada header como se desee
            case STRING_DIEGO_MDJ:

                // recibir_string recibe un stream de datos del socket del cual se envio el mensaje y los interpreta
                // como string, agregando \0 al final y metiendo los datos en el array str
                recibir_string(mensaje.socket, str, TAMANIO_MAXIMO_STRING);
                printf("MDJ recibio: %s\n", (char*)str);

                //si recibe el string "exit", MDJ se cierra
                if(!strcmp(str, "exit"))
                    cerrar_mdj(logger, configuracion, server);
                break;

            case CONEXION_CERRADA:
                // el header CONEXION_CERRADA indica que el que nos envio ese mensaje se desconecto, idealmente los
                // procesos que cierran deberian mandar este header antes de hacerlo para que los procesos a los cuales
                // estan conectados se enteren

                if(mensaje.t_proceso==t_elDiego)
                    cerrar_mdj(logger, configuracion, server);

            default:
                // TODO aca habria que programar que pasa si se manda un header invalido
                break;
        }
    }
}
