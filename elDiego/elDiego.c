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
#include <ensalada/validacion.h>
#include <ensalada/com.h>

struct sockaddr_in addr_mdj;

int main(int argc, char **argv) {
	validar_parametros(argc);
	t_config* cfg_file = validar_config(argv[1],elDiego);
	cfg_elDiego* configuracion = asignar_config(cfg_file,elDiego);

	int socket_mdj = crearSocket();
	char* buffer;
	int tamanio = 0;

	inicializarDireccion(&addr_mdj,configuracion->puerto_mdj,configuracion->ip_mdj);
	conectar_Servidor(socket_mdj,&addr_mdj);
	handshakeCliente(t_elDiego,socket_mdj);
	printf("Coneccion con Mercado De Jugadores exitosa!\n");

	for(;;){
		printf("Ingrese un string para enviarlo al MDJ\n");
		buffer = malloc(0);
		while(1){
			buffer = realloc(buffer,tamanio + 1);
			int c = getchar();
			if( c == '\n' ){
			  buffer[tamanio] = '\0';

			  break;
			}
			buffer[tamanio] = (char)c;
			tamanio++;
		}

		send(socket_mdj,&tamanio,sizeof(int),0);
		send(socket_mdj,buffer,tamanio,0);
		tamanio = 0;
		free(buffer);
		system("clear");
	}
	puts(""); /* prints  */
	return EXIT_SUCCESS;
}
