#ifndef MENSAJE_H
#define MENSAJE_H

#include <stdlib.h>
#include <string.h>
#include <commons/collections/queue.h>
#include <sys/socket.h>


typedef struct {
    int longitud;
    void* datos;

}NodoPayload;

typedef struct {
    int header;
    int longitud;
    int socket_destino;
    t_queue* payload;

}MensajeDinamico;

MensajeDinamico* crear_mensaje(int, int);
void destruir_mensaje(MensajeDinamico* mensaje);
void agregar_dato(MensajeDinamico*, int, void*);
void agregar_string(MensajeDinamico*, char*);
int enviar_mensaje(MensajeDinamico*);
MensajeDinamico* recibir_mensaje(MensajeEntrante metadata);


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
MensajeDinamico* crear_mensaje_mdj_crear_archivo(int socket_destino, char* path);

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

#endif //MENSAJE_H
