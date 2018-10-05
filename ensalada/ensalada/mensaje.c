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
    mensaje->longitud = sizeof(int)*2;
    mensaje->socket = socket_destino;

    return mensaje;
}

void destruir_elemento_cola(void* arg){
    NodoPayload* elemento = (NodoPayload*)arg;

    free(elemento->datos);
    free(elemento);
}

void destruir_mensaje(MensajeDinamico* mensaje){
	NodoPayload* nodo_aux;

	while(queue_size(mensaje->payload)){
	    nodo_aux = queue_pop(mensaje->payload);

	    free(nodo_aux->datos);
	    free(nodo_aux);
	}
    queue_destroy(mensaje->payload);
	free(mensaje);
}

/*!
 * Pushea dato y su longitud a la cola de datos a enviar
 * @param mensaje MensajeDinamico al cual agregar el dato
 * @param longitud longitud del dato a agregar
 * @param dato puntero al dato a agregar
 */
void agregar_dato(MensajeDinamico* mensaje, int longitud, void* dato){
    NodoPayload* nuevo_nodo = malloc(sizeof(NodoPayload));
    void* buffer_dato = malloc((size_t)longitud);
    memcpy(buffer_dato, dato, (size_t)longitud);

    nuevo_nodo->datos = buffer_dato;
    nuevo_nodo->longitud = longitud;
    queue_push(mensaje->payload, nuevo_nodo);

    mensaje->longitud+=longitud+sizeof(int);
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
 * @return retorna lo que devuelve send
 */
int enviar_mensaje(MensajeDinamico* mensaje){
    int resultado, posicion_buffer = 0;
    NodoPayload* buffer_nodo;
    void* buffer_mensaje;
    int longitud;
    memcpy(&longitud, &mensaje->longitud, sizeof(int));

    buffer_mensaje = malloc((size_t)mensaje->longitud);
    memcpy(buffer_mensaje, &(mensaje->header), sizeof(int));
    posicion_buffer += sizeof(int);

	memcpy(buffer_mensaje+posicion_buffer, &(mensaje->longitud), sizeof(int));
    posicion_buffer += sizeof(int);

    while(!queue_is_empty(mensaje->payload)){
        buffer_nodo = queue_pop(mensaje->payload);

		memcpy(buffer_mensaje+posicion_buffer, &(buffer_nodo->longitud), sizeof(int));
        posicion_buffer += sizeof(int);

		memcpy(buffer_mensaje+posicion_buffer, buffer_nodo->datos, (size_t)buffer_nodo->longitud);
        posicion_buffer += buffer_nodo->longitud;

        free(buffer_nodo->datos);
        free(buffer_nodo);
    }

    resultado = send(mensaje->socket, buffer_mensaje, (size_t)mensaje->longitud, 0);
    destruir_mensaje(mensaje);
    free(buffer_mensaje);

    if(resultado == longitud)
        return 1;
    else
        return -1;

}

MensajeDinamico* recibir_mensaje(int header, int socket){
	MensajeDinamico* mensaje = crear_mensaje(header, socket);

	int longitud = 0;
	int recibido = 0;
	char* buffer;

    comprobar_error(recv(socket, &longitud, sizeof(int), MSG_WAITALL), "Error al recibir longitud mensaje");
    mensaje->longitud = longitud;

	longitud -= sizeof(int)*2;

	while(recibido < longitud){
		int tamanio = 0;
		recibido += recv(socket,&tamanio,sizeof(int),MSG_WAITALL);
		buffer = malloc((size_t)tamanio);
		recibido += recv(socket,buffer,(size_t)tamanio,MSG_WAITALL);
		agregar_dato(mensaje,tamanio,buffer);
		free(buffer);
	}

	return mensaje;
}

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
	MensajeDinamico* mensaje_dinamico = crear_mensaje(VALIDAR_ARCHIVO,socket_destino);
	agregar_dato(mensaje_dinamico,strlen(path),path);
	return mensaje_dinamico;
}

MensajeDinamico* crear_mensaje_mdj_crear_archivo(int socket_destino, char* path, int cantidad_lineas){
	MensajeDinamico* mensaje_dinamico = crear_mensaje(CREAR_ARCHIVO,socket_destino);
	agregar_dato(mensaje_dinamico,sizeof(int),&cantidad_lineas);
	agregar_dato(mensaje_dinamico,strlen(path),path);
	return mensaje_dinamico;
}

MensajeDinamico* crear_mensaje_mdj_obtener_datos(int socket_destino, char* path, int offset,int size){
	MensajeDinamico* mensaje_dinamico = crear_mensaje(OBTENER_DATOS,socket_destino);
	agregar_dato(mensaje_dinamico,sizeof(int),&size);
	agregar_dato(mensaje_dinamico,sizeof(int),&offset);
	agregar_dato(mensaje_dinamico,strlen(path),path);
	return mensaje_dinamico;
}

MensajeDinamico* crear_mensaje_mdj_guardar_datos(int socket_destino, char* path, int offset, int size, char* buffer){
	MensajeDinamico* mensaje_dinamico = crear_mensaje(OBTENER_DATOS,socket_destino);
	agregar_dato(mensaje_dinamico,strlen(buffer),buffer);
	agregar_dato(mensaje_dinamico,sizeof(int),&size);
	agregar_dato(mensaje_dinamico,sizeof(int),&offset);
	agregar_dato(mensaje_dinamico,strlen(path),path);
	return mensaje_dinamico;
}
MensajeDinamico* crear_mensaje_mdj_borrar_archivo(int socket_destino, char* path){
	MensajeDinamico* mensaje_dinamico = crear_mensaje(BORRAR_ARCHIVO,socket_destino);
	agregar_dato(mensaje_dinamico,strlen(path),path);
	return mensaje_dinamico;
}
