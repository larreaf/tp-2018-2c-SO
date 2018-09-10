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
void agregar_dato(MensajeDinamico*, int, void*);
int enviar_mensaje(MensajeDinamico*);

#endif MENSAJE_H
