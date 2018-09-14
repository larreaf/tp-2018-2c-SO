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
    STRING_DIEGO_FM9,
    STRING_MDJ_DIEGO,
    STRING_CONSOLA_PROPIA,
    OPERACION_CONSOLA_TERMINADA
}protocolo_header;

int recibir_string(int, char*, int);

#endif /* PROTOCOLO_H_ */
