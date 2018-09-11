#include "protocolo.h"


int enviar_mensaje(int socket_e, char* buffer){
	return 0;
}

int recibir_mensaje(int socket_r, char* buffer){
	int valor_retorno = 0;
	protocolo_header header = 0;
	valor_retorno = recv(socket_r,&header,sizeof(int),0);
	switch(header){
		case HANDSHAKE_CLIENTE:
			break;
		case STRING_DIEGO_MDJ:
			break;
		default:
			break;
	}
	return valor_retorno;
}

void recibir_string(int socket, char* linea){
    char *buffer_linea;
    int tamanio_linea;

    recv(socket, &tamanio_linea, sizeof(int), 0);

    // Reservo memoria para la linea + 1 byte para el \0, luego recibo la linea y la imprimo
    buffer_linea = malloc((size_t) tamanio_linea + 1);
    recv(socket, buffer_linea, (size_t) tamanio_linea, 0);
    buffer_linea[tamanio_linea] = '\0';

    strcpy(linea, buffer_linea);
    free(buffer_linea);
}
