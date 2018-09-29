#ifndef PROTOCOLO_H_
#define PROTOCOLO_H_

#include <sys/socket.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>


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
	VALIDAR_ARCHIVO,
	CREAR_ARCHIVO,
	OBTENER_DATOS,
	GUARDAR_DATOS,
    OPERACION_CONSOLA_TERMINADA
}protocolo_header;

typedef enum {
    OK,
    ERROR,
    WARNING
}tipo_mensaje;

typedef struct{
    tipo_mensaje tipo;
    char detalle[200];
}mensaje;

char* recibir_string(int);

#endif /* PROTOCOLO_H_ */
