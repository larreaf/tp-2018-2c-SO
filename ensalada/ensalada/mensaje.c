#include "mensaje.h"

/*!
 * Crea mensaje en memoria con el header elegido y crea la cola que almacenara los datos a enviar y sus longitudes
 * @param header int correspondiente al header que se desea enviar
 * @param socket_destino socket al cual se va a mandar el mensaje
 * @return puntero al MensajeDinamico creado
 */
MensajeDinamico* crear_mensaje(int header, int socket_destino){
    MensajeDinamico *mensaje = malloc(sizeof(MensajeDinamico));
    mensaje->header = header;
    mensaje->payload = queue_create();
    mensaje->longitud = sizeof(int);
    mensaje->socket_destino = socket_destino;

    return mensaje;
}

/*!
 * Pushea dato y su longitud a la cola de datos a enviar
 * @param mensaje MensajeDinamico al cual agregar el dato
 * @param longitud longitud del dato a agregar
 * @param dato puntero al dato a agregar
 */
void agregar_dato(MensajeDinamico* mensaje, int longitud, void* dato){
    NodoPayload* nuevo_nodo = malloc(sizeof(NodoPayload));
    nuevo_nodo->datos = dato;
    nuevo_nodo->longitud=longitud;
    queue_push(mensaje->payload, nuevo_nodo);
    mensaje->longitud+=sizeof(int)+longitud;
}

/*!
 * Envia el mensaje al socket elegido cuando se creo y libera memoria
 * La estructura del mensaje sera siempre: int header, int longitud dato, puntero a dato,
 * int longitud dato, puntero dato, etc... como tantos datos de hallan agregado al mensaje
 * @param puntero a MensajeDinamico a enviar
 * @return retorna lo que devuelve send
 */
int enviar_mensaje(MensajeDinamico* mensaje){
    int resultado, posicion_buffer = 0;
    NodoPayload* buffer_nodo;
    void* buffer_mensaje;

    buffer_mensaje = malloc((size_t)mensaje->longitud);
    memmove(buffer_mensaje, &(mensaje->header), sizeof(int));
    posicion_buffer += sizeof(int);

    while(!queue_is_empty(mensaje->payload)){
        buffer_nodo = queue_pop(mensaje->payload);
        memmove(buffer_mensaje+posicion_buffer, &(buffer_nodo->longitud), sizeof(int));
        posicion_buffer += sizeof(int);
        memmove(buffer_mensaje+posicion_buffer, buffer_nodo->datos, (size_t)buffer_nodo->longitud);
        posicion_buffer += buffer_nodo->longitud;
        free(buffer_nodo);
    }

    queue_destroy(mensaje->payload);
    resultado = send(mensaje->socket_destino, buffer_mensaje, (size_t)mensaje->longitud, 0);
    free(buffer_mensaje);
    free(mensaje);

    return resultado;
}
