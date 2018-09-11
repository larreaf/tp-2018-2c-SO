#ifndef PROTOCOLO_H_
#define PROTOCOLO_H_

#include <sys/socket.h>
typedef enum {
    HANDSHAKE_CLIENTE,
    CONEXION_CERRADA,
    STRING_DIEGO_MDJ,
    STRING_DIEGO_FM9
}protocolo_header;

int enviar_mensaje();
int recibir_mensaje();

#endif /* PROTOCOLO_H_ */
