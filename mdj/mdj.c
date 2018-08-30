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
#include <ensalada/validacion.h>
#include <ensalada/com.h>
#include <netinet/in.h>

struct sockaddr_in addr_local;

int socket_elDiego = 0;
int socket_escucha = 0;
int main(int argc, char **argv) {

	char* buffer;
	int tamanio = 0;

	validar_parametros(argc);
	t_config* cfg_file = validar_config(argv[1],mdj);
	cfg_mdj* configuracion = asignar_config(cfg_file,mdj);
	config_destroy(cfg_file);
	inicializarDireccion(&addr_local,configuracion->puerto,MY_IP);
	socket_escucha = escuchar_Conexion((&addr_local));
	socket_elDiego = aceptar_conexion(socket_escucha);
	Proceso cliente = handshakeServidor(socket_elDiego);
	if(cliente != t_elDiego){
		close(socket_elDiego);
		exit(-1);
	}
	for(;;){
		recv(socket_elDiego,&tamanio,sizeof(int),0);

		buffer = malloc(tamanio);

		recv(socket_elDiego,buffer,tamanio,0);
		buffer[tamanio] = '\0';
		printf("recibio: %s\n",buffer);
		if(!strcmp(buffer,"exit")){
			free(buffer);
			free(configuracion);
			exit(0);
		}
		free(buffer);
		tamanio = 0;
	}


	puts("fin"); /* prints  */
	return EXIT_SUCCESS;
}
