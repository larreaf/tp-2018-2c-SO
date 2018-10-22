#ifndef MENSAJE_H
#define MENSAJE_H

#include <stdlib.h>
#include <string.h>
#include <commons/collections/queue.h>
#include <commons/string.h>
#include <sys/socket.h>
#include "protocolo.h"

#define min( a, b ) ( ((a) < (b)) ? (a) : (b) )

typedef enum {
    t_safa,
    t_elDiego,
    t_fm9,
    t_cpu,
    t_mdj,
    t_consola_mdj,
    t_consola_safa,
    t_consola_fm9,
    cantidad_tipos_procesos // este tiene que ir ultimo en el enum SIEMPRE o hacemos quilombo
} Proceso;

typedef struct {
    t_queue* cola_particiones_dato;
    int tamanio_cola_particiones_dato;
}DatoParticionado;

typedef struct {
    int longitud;
    void* datos;
}NodoPayload;

typedef struct {
    int header;
    int longitud;
    int socket;
    int particionado;
    t_queue* payload;
    Proceso t_proceso;
}MensajeDinamico;

MensajeDinamico* crear_mensaje(int, int, int);
void destruir_mensaje(MensajeDinamico* mensaje);
void agregar_dato(MensajeDinamico*, int, void*);
void recibir_int(int*, MensajeDinamico*);
void recibir_string(char**, MensajeDinamico*);
void agregar_string(MensajeDinamico*, char*);
DatoParticionado particionar_dato(void*, int, int);
int enviar_mensaje(MensajeDinamico*);
MensajeDinamico* recibir_mensaje(int);

/*
 * @NAME: crear_mensaje_mdj_validar_archivo
 * @DESC: crea el mensaje para enviar request a mdj para validar un archivo
 * @ARG: socket de mdj + path del archivo
 */
MensajeDinamico* crear_mensaje_mdj_validar_archivo(int socket_destino, char* path);

/*
 * @NAME: crear_mensaje_mdj_crear_archivo
 * @DESC: crea el mensaje para enviar request a mdj para crear un archivo
 * @ARG: socket de mdj + path del archivo
 */
MensajeDinamico* crear_mensaje_mdj_crear_archivo(int socket_destino, char* path, int* cantidad_lineas);

/*
 * @NAME: crear_mensaje_mdj_obtener_datos
 * @DESC: crea el mensaje para enviar request a mdj para obtener datos
 * @ARG: socket de mdj
 * 		+ path del archivo
 * 		+ offset: desde donde leer
 *		+ size: tamanio a leer
 */
MensajeDinamico* crear_mensaje_mdj_obtener_datos(int socket_destino, char* path, int offset, int size);

/*
 * @NAME: crear_mensaje_mdj_guardar_datos
 * @DESC: crea el mensaje para enviar request a mdj para guardar datos
 * @ARG: socket de mdj
 * 		+ path del archivo
 * 		+ offset: desde donde leer
 *		+ size: tamanio a leer
 *		+ buffer: datos a guardar
 */
MensajeDinamico* crear_mensaje_mdj_guardar_datos(int socket_destino, char* path, int offset, int size, char* buffer);

/*
 * @NAME: crear_mensaje_mdj_borrar_archivo
 * @DESC: crea el mensaje para enviar request a mdj para borrar un archivo de mdj
 * @ARG: socket de mdj + path del archivo
 */
MensajeDinamico* crear_mensaje_mdj_borrar_archivo(int socket_destino, char* path);

#endif //MENSAJE_H
