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

/*!
 * Recibe un stream de bytes y lo guarda como string
 * @param socket el socket a leer
 * @param linea array de char para almacenar el string, si no entra copia nada mas \0
 * @param tamanio_maximo_string int que indica el tamanio del array linea para no sobrepasarlo
 */
void recibir_string(int socket, char* linea, int tamanio_maximo_string){
    char *buffer_string;
    int tamanio_string;

    recv(socket, &tamanio_string, sizeof(int), 0);

    // Reservo memoria para la linea + 1 byte para el \0, luego recibo la linea y la imprimo
    buffer_string = malloc((size_t) tamanio_string + 1);
    recv(socket, buffer_string, (size_t) tamanio_string, 0);
    buffer_string[tamanio_string] = '\0';

    // Verificar que el char* pasado tenga suficiente espacio para almacenar el string, sino segmentation fault
    if(tamanio_string<tamanio_maximo_string)
        strcpy(linea, buffer_string);
    else
        strcpy(linea, "\0");

    free(buffer_string);
}
