/*
 ============================================================================
 Name        : elDiego.c
 Author      : 
 Version     :
 Copyright   : 
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include "validacion.h"
#include "com.h"
#include "mensaje.h"
#include <readline/readline.h>
#include "protocolo.h"
#include <string.h>
#include <commons/log.h>
#include "servidor.h"

int main(int argc, char **argv) {
    int socket_mdj, socket_fm9, retorno_send, header, conexiones_permitidas[cantidad_tipos_procesos]={0};
    t_log* logger;
    MensajeDinamico* mensaje;
    Servidor servidor;

	validar_parametros(argc);
	cfg_elDiego* configuracion = asignar_config(argv[1],elDiego);

	logger = log_create("elDiego.log", "elDiego", true, log_level_from_string("info"));

	conexiones_permitidas[t_cpu] = 3;
	servidor = inicializar_servidor(logger, configuracion->puerto, conexiones_permitidas, t_elDiego);

	// conectar como cliente a MDJ
	socket_mdj = conectar_como_cliente(servidor, configuracion->ip_mdj, configuracion->puerto_mdj, t_mdj);

	// conectar como cliente a FM9
	socket_fm9 = conectar_como_cliente(servidor, configuracion->ip_fm9, configuracion->puerto_fm9, t_fm9);

    while(1){

        // este codigo es de prueba, no incluye ninguna funcionalidad de elDiego, despues lo borramos

        char* linea = readline("> ");

        if(!strncmp(linea, "fm9", 3)){
            mensaje = crear_mensaje(STRING_DIEGO_FM9, socket_fm9);
        }else if(!strncmp(linea, "mdj", 3)){
            mensaje = crear_mensaje(STRING_DIEGO_MDJ, socket_mdj);
        }else{
            if(!strcmp(linea, "exit")){
                destroy_cfg(configuracion,t_elDiego);
                free(linea);
                header = CONEXION_CERRADA;
                send(socket_fm9, &header, sizeof(CONEXION_CERRADA), 0);
                send(socket_mdj, &header, sizeof(CONEXION_CERRADA), 0);
                destruir_servidor(servidor);
                exit(0);
            }else{
                free(linea);
                continue;
            }

        }

        agregar_string(mensaje, linea+4);
        retorno_send = enviar_mensaje(mensaje);
        printf("%d\n", retorno_send);

        /*
        nuevo_mensaje = esperar_mensajes(servidor);
        if(nuevo_mensaje.header==STRING_MDJ_DIEGO){
            str = recibir_string(nuevo_mensaje.socket);
            printf("Diego recibio: %s\n", str);
            free(str);
        }
         */
    }
}
