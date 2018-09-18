#ifndef PROTOCOLO_H_
#define PROTOCOLO_H_

#include <sys/socket.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "com.h"

typedef enum {
    HANDSHAKE_CLIENTE,
    CONEXION_CERRADA,
    STRING_DIEGO_MDJ,
    STRING_DIEGO_FM9,
    STRING_MDJ_DIEGO,
    STRING_CPU_SAFA,
    STRING_DIEGO_SAFA,
    STRING_CONSOLA_PROPIA,
    STRING_SAFA_DIEGO,
    OPERACION_CONSOLA_TERMINADA
}protocolo_header;

char* recibir_string(int);

#endif /* PROTOCOLO_H_ */
