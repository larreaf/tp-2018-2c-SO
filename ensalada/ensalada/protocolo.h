#ifndef PROTOCOLO_H_
#define PROTOCOLO_H_

#include <sys/socket.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define TAMANIO_MAXIMO_STRING 128

typedef enum {
    HANDSHAKE_CLIENTE,
    CONEXION_CERRADA,
    STRING_DIEGO_MDJ,
    STRING_DIEGO_FM9
}protocolo_header;

int enviar_mensaje();
int recibir_mensaje();
void recibir_string(int, char*, int);

#endif /* PROTOCOLO_H_ */
