#include "mensaje.h"

/*!
 * Crea mensaje en memoria con el header elegido y crea la cola que almacenara los datos a enviar y sus longitudes
 * @param header int correspondiente al header que se desea enviar
 * @param socket_destino socket al cual se va a mandar el mensaje
 * @return puntero al MensajeDinamico creado
 */
MensajeDinamico* crear_mensaje(int header, int socket_destino, int particionado){
    MensajeDinamico *mensaje = malloc(sizeof(MensajeDinamico));
    mensaje->header = header;
    mensaje->payload = queue_create();
    mensaje->longitud = sizeof(int)*3;
    mensaje->socket = socket_destino;
    mensaje->particionado = particionado;

    return mensaje;
}

/*!
 * Destruye un MensajeDinamico, liberando memoria
 * @param mensaje MensajeDinamicoa destruir
 */
void destruir_mensaje(MensajeDinamico* mensaje){
	NodoPayload* nodo_aux, *nodo_aux_particion;

	if(!mensaje->particionado) {
        while (!queue_is_empty(mensaje->payload)) {
            nodo_aux = queue_pop(mensaje->payload);

            free(nodo_aux->datos);
            free(nodo_aux);
        }
    }else {
        while (queue_size(mensaje->payload)) {
            nodo_aux = queue_pop(mensaje->payload);

            while(!queue_is_empty(nodo_aux->datos)){
                nodo_aux_particion = queue_pop(nodo_aux->datos);
                free(nodo_aux_particion->datos);
                free(nodo_aux_particion);
            }
            queue_destroy(nodo_aux->datos);
            free(nodo_aux);
        }
    }
    queue_destroy(mensaje->payload);
	free(mensaje);
}

/*!
 * Particiona un dato en particiones de igual longitud
 * @param dato puntero al dato a particionar
 * @param longitud longitud del dato a particionar
 * @param longitud_particion longitud de la particion
 * @return struct DatoParticionado
 */
DatoParticionado particionar_dato(void* dato, int longitud, int longitud_particion){
    t_queue* cola_particiones_dato = queue_create();
    DatoParticionado dato_particionado;
    NodoPayload* nuevo_nodo;
    void* buffer_dato;
    int restante;

    dato_particionado.cola_particiones_dato = cola_particiones_dato;
    dato_particionado.tamanio_cola_particiones_dato = 0;

    for(int i = 0; (longitud-i)>0; i+=longitud_particion){
        nuevo_nodo = malloc(sizeof(NodoPayload));
        restante = min(longitud_particion, longitud-i);
        buffer_dato = malloc((size_t)restante);

        nuevo_nodo->longitud = restante;
        memcpy(buffer_dato, dato+i, (size_t)restante);

        nuevo_nodo->datos = buffer_dato;
        queue_push(cola_particiones_dato, nuevo_nodo);

        dato_particionado.tamanio_cola_particiones_dato += sizeof(int);
        dato_particionado.tamanio_cola_particiones_dato += restante;
    }
    return dato_particionado;
}

/*!
 * Pushea dato y su longitud a la cola de datos a enviar
 * @param mensaje MensajeDinamico al cual agregar el dato
 * @param longitud longitud del dato a agregar
 * @param dato puntero al dato a agregar
 */
void agregar_dato(MensajeDinamico* mensaje, int longitud, void* dato){
    NodoPayload* nuevo_nodo = malloc(sizeof(NodoPayload));
    DatoParticionado dato_particionado;

    if(!mensaje->particionado) {
        void* buffer_dato = malloc((size_t)longitud);
        memcpy(buffer_dato, dato, (size_t)longitud);
        nuevo_nodo->datos = buffer_dato;
        nuevo_nodo->longitud = longitud;
        queue_push(mensaje->payload, nuevo_nodo);

        mensaje->longitud += longitud + sizeof(int);
    }else{
        dato_particionado = particionar_dato(dato, longitud, mensaje->particionado);

        nuevo_nodo->datos = dato_particionado.cola_particiones_dato;
        nuevo_nodo->longitud = longitud;
        queue_push(mensaje->payload, nuevo_nodo);

        mensaje->longitud+=dato_particionado.tamanio_cola_particiones_dato+sizeof(int);
    }
}

/*!
 * agrega un string al mensaje dinamico
 * @param mensaje mensaje dinamico al cual agregar el string
 * @param str string a agregar
 */
void agregar_string(MensajeDinamico* mensaje, char* str){
    agregar_dato(mensaje, strlen(str)+1, str);
}

/*!
 * Envia el mensaje al socket elegido cuando se creo y libera memoria
 * La estructura del mensaje sera siempre: int header, int longitud dato, puntero a dato,
 * int longitud dato, puntero dato, etc... como tantos datos de hallan agregado al mensaje
 * @param puntero a MensajeDinamico a enviar
 * @return retorna 1 si se pudo enviar el mensaje o -1 si hubo algun error
 */
int enviar_mensaje(MensajeDinamico* mensaje){
    int resultado = 0, posicion_buffer = 0;
    NodoPayload* buffer_nodo, *buffer_particion;
    void* buffer_mensaje;
    int longitud, enviado;
    memcpy(&longitud, &mensaje->longitud, sizeof(int));

    if(!mensaje->particionado) {
        buffer_mensaje = malloc((size_t) mensaje->longitud);
        memcpy(buffer_mensaje, &(mensaje->header), sizeof(int));
        posicion_buffer += sizeof(int);

        memcpy(buffer_mensaje + posicion_buffer, &(mensaje->longitud), sizeof(int));
        posicion_buffer += sizeof(int);

        memcpy(buffer_mensaje + posicion_buffer, &(mensaje->particionado), sizeof(int));
        posicion_buffer += sizeof(int);

        while (!queue_is_empty(mensaje->payload)) {
            buffer_nodo = queue_pop(mensaje->payload);

            memcpy(buffer_mensaje + posicion_buffer, &(buffer_nodo->longitud), sizeof(int));
            posicion_buffer += sizeof(int);

            memcpy(buffer_mensaje + posicion_buffer, buffer_nodo->datos, (size_t) buffer_nodo->longitud);
            posicion_buffer += buffer_nodo->longitud;

            free(buffer_nodo->datos);
            free(buffer_nodo);
        }

        resultado = send(mensaje->socket, buffer_mensaje, (size_t) mensaje->longitud, 0);
        free(buffer_mensaje);
    }else{
        resultado+=send(mensaje->socket, &mensaje->header, sizeof(int), 0);
        resultado+=send(mensaje->socket, &mensaje->longitud, sizeof(int), 0);
        resultado+=send(mensaje->socket, &mensaje->particionado, sizeof(int), 0);

        while (!queue_is_empty(mensaje->payload)) {
            buffer_nodo = queue_pop(mensaje->payload);
            resultado+=send(mensaje->socket, &buffer_nodo->longitud, sizeof(int), 0);

            for(int i = 0; i<buffer_nodo->longitud; i+=enviado, resultado+=enviado) {
                buffer_particion = queue_pop(buffer_nodo->datos);
                resultado+=send(mensaje->socket, &buffer_particion->longitud, sizeof(int), 0);
                enviado = send(mensaje->socket, buffer_particion->datos, (size_t)buffer_particion->longitud, 0);
                free(buffer_particion->datos);
                free(buffer_particion);
            }
            queue_destroy(buffer_nodo->datos);
            free(buffer_nodo);
        }
    }

    destruir_mensaje(mensaje);

    if(resultado == longitud)
        return 1;
    else
        return -1;

}

/*!
 * Recibe un mensaje de un socket
 * @param header header que se recibio
 * @param socket socket de donde recibir los datos
 * @return puntero a un MensajeDinamico
 */
MensajeDinamico* recibir_mensaje(int header, int socket){
	MensajeDinamico* mensaje = crear_mensaje(header, socket, 0);
	int longitud_mensaje_total = 0, particionado, recibido_particion = 0, recibido_total = 0, buffer_longitud_particion,
	recibido;
	char* buffer;

    comprobar_error(recv(socket, &longitud_mensaje_total, sizeof(int), MSG_WAITALL), "Error al recibir longitud total"
                                                                                     "de mensaje");

    comprobar_error(recv(socket, &particionado, sizeof(int), MSG_WAITALL), "Error al recibir flag particionado");

	longitud_mensaje_total -= sizeof(int)*3;

	if(!particionado) {
        while (recibido_particion < longitud_mensaje_total) {
            int tamanio = 0;
            recibido_particion += recv(socket, &tamanio, sizeof(int), MSG_WAITALL);
            buffer = malloc((size_t) tamanio);
            recibido_particion += recv(socket, buffer, (size_t) tamanio, MSG_WAITALL);
            agregar_dato(mensaje, tamanio, buffer);
            free(buffer);
        }
    }else{
	    while(recibido_total < longitud_mensaje_total){
            int tamanio = 0;
            recibido_particion = 0;
            recibido_total += recv(socket, &tamanio, sizeof(int), MSG_WAITALL);
            buffer = malloc((size_t)tamanio);

            while(recibido_particion<tamanio){
                recibido_total += recv(socket, &buffer_longitud_particion, sizeof(int), MSG_WAITALL);
                recibido = recv(socket, buffer+recibido_particion, (size_t)buffer_longitud_particion, MSG_WAITALL);
                recibido_particion += recibido;
                recibido_total += recibido;
            }
            agregar_dato(mensaje, tamanio, buffer);
            free(buffer);
	    }
	}
	return mensaje;
}

/*!
 * Desempaca un int de un MensajeDinamico
 * @param destino puntero al int en donde desempacar
 * @param mensaje mensaje dinamico del cual desempacar el int
 */
void recibir_int(int* destino, MensajeDinamico* mensaje){
    NodoPayload* nodo_aux = queue_pop(mensaje->payload);
    memcpy(destino, nodo_aux->datos, sizeof(int));
    free(nodo_aux->datos);
    free(nodo_aux);
}

void recibir_string(char** destino, MensajeDinamico* mensaje){
    NodoPayload* nodo_aux = queue_pop(mensaje->payload);

    *destino = string_new();
    string_append(destino, nodo_aux->datos);
    free(nodo_aux->datos);
    free(nodo_aux);
}

MensajeDinamico* crear_mensaje_mdj_validar_archivo(int socket_destino, char* path){
	MensajeDinamico* mensaje_dinamico = crear_mensaje(VALIDAR_ARCHIVO,socket_destino, 0);
	agregar_dato(mensaje_dinamico,strlen(path),path);
	return mensaje_dinamico;
}

MensajeDinamico* crear_mensaje_mdj_crear_archivo(int socket_destino, char* path, int* cantidad_lineas){
	MensajeDinamico* mensaje_dinamico = crear_mensaje(CREAR_ARCHIVO,socket_destino, 0);
	agregar_string(mensaje_dinamico, path);
	agregar_dato(mensaje_dinamico,sizeof(int),cantidad_lineas);
	return mensaje_dinamico;
}

MensajeDinamico* crear_mensaje_mdj_obtener_datos(int socket_destino, char* path, int offset,int size){
	MensajeDinamico* mensaje_dinamico = crear_mensaje(OBTENER_DATOS,socket_destino, 0);
	agregar_string(mensaje_dinamico, path);
	agregar_dato(mensaje_dinamico,sizeof(int),&offset);
	agregar_dato(mensaje_dinamico,sizeof(int),&size);
	return mensaje_dinamico;
}

MensajeDinamico* crear_mensaje_mdj_guardar_datos(int socket_destino, char* path, int offset, int size, char* buffer){
	MensajeDinamico* mensaje_dinamico = crear_mensaje(GUARDAR_DATOS,socket_destino, 0);
	agregar_string(mensaje_dinamico, path);
	agregar_dato(mensaje_dinamico,sizeof(int),&size);
	agregar_dato(mensaje_dinamico,sizeof(int),&offset);
	agregar_dato(mensaje_dinamico,strlen(buffer),buffer);
	return mensaje_dinamico;
}
MensajeDinamico* crear_mensaje_mdj_borrar_archivo(int socket_destino, char* path){
	MensajeDinamico* mensaje_dinamico = crear_mensaje(BORRAR_ARCHIVO,socket_destino, 0);
	agregar_dato(mensaje_dinamico,strlen(path),path);
	return mensaje_dinamico;
}
