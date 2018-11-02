#ifndef PROTOCOLO_H_
#define PROTOCOLO_H_

#include <sys/socket.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>


typedef enum {
    HANDSHAKE_CLIENTE,
    CONEXION_CERRADA,
    NUEVA_CONEXION,
    STRING_DIEGO_MDJ,
    STRING_DIEGO_FM9_BUSCAR,
	STRING_DIEGO_FM9_ESCRIBIR,
	STRING_DIEGO_FM9_MODIFICAR,
	STRING_DIEGO_FM9_CERRAR,
    STRING_MDJ_DIEGO,
    STRING_CPU_SAFA,
    STRING_DIEGO_SAFA,
    STRING_CONSOLA_PROPIA,
    STRING_SAFA_DIEGO,
    OPERACION_CONSOLA_TERMINADA,
    DATOS_DTB,
	VALIDAR_ARCHIVO,
	CREAR_ARCHIVO,
	OBTENER_DATOS,
	GUARDAR_DATOS,
	BORRAR_ARCHIVO,
    ABRIR_SCRIPT_CPU_DIEGO,
    CARGAR_SCRIPT,
    RESULTADO_CARGAR_SCRIPT,
    RESULTADO_CARGAR_ARCHIVO,
    PASAR_DTB_A_READY,
    DESBLOQUEAR_DTB,
    ABRIR_ARCHIVO_CPU_DIEGO,
	CERRAR_ARCHIVO_CPU_FM9,
	CREAR_ARCHIVO_CPU_DIEGO,
	BORRAR_ARCHIVO_CPU_DIEGO,
	ASIGNAR_ARCHIVO_CPU_FM9,
	LEER_LINEA,
	RESULTADO_LEER_LINEA,
	CARGAR_ARCHIVO,
	FLUSH_ARCHIVO,
	RESULTADO_FLUSH_ARCHIVO
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

/*
 * @NAME: compruebaError
 * @DESC: comprueba los errores en las funciones de sockets y emite un mensaje personalizado en caso de error.
 * Tambien aborta el proceso.
 */
void comprobar_error(int variable, const char* mensajeError);

#endif /* PROTOCOLO_H_ */
