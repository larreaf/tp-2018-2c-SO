#include "protocolo.h"


int enviar_mensaje(int socket_e, char* buffer){
	return 0;
}

int recibir_mensaje(int socket_r, char* buffer){
	int valor_retorno = 0;
	protocolo_header header = 0;
	valor_retorno = recv(socket_r,&header,sizeof(int),0);
	switch(header){
		case example1:
			break;
		case example2:
			break;
		default:
			break;
	}
	return valor_retorno;
}

