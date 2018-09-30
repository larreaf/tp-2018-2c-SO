

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

void destruir_mensaje(MensajeDinamico* mensaje){
	void destroy_nodo(void* element){
		free(element);
	}
	queue_destroy_and_destroy_elements(mensaje->payload,&destroy_nodo);
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
    mensaje->longitud+=sizeof(int)+longitud;
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

    mensaje->longitud += sizeof(int); //Por el envio de la longitud total despues del header


    buffer_mensaje = malloc((size_t)mensaje->longitud);
    memmove(buffer_mensaje, &(mensaje->header), sizeof(int));
    posicion_buffer += sizeof(int);

    memmove(buffer_mensaje, &(mensaje->longitud), sizeof(int));
    posicion_buffer += sizeof(int);

    while(!queue_is_empty(mensaje->payload)){
        buffer_nodo = queue_pop(mensaje->payload);
        memmove(buffer_mensaje+posicion_buffer, &(buffer_nodo->longitud), sizeof(int));
        posicion_buffer += sizeof(int);
        memmove(buffer_mensaje+posicion_buffer, buffer_nodo->datos, (size_t)buffer_nodo->longitud);
        posicion_buffer += buffer_nodo->longitud;
        free(buffer_nodo->datos);
        free(buffer_nodo);
    }
    resultado = send(mensaje->socket_destino, buffer_mensaje, (size_t)mensaje->longitud, 0);
    free(buffer_mensaje);
    destruir_mensaje(mensaje);

    return resultado;
}

MensajeDinamico* recibir_mensaje(MensajeEntrante metadata){
	MensajeDinamico mensaje = crear_mensaje(metadata.header,metadata.socket);

	int longitud = 0;
	int recibido = 0;
	char* buffer;

	comprobar_error(recv(metadata.socket,&longitud,sizeof(int)),"Error");
	metadata.longitud = longitud - 2* sizeof(int);
	while(recibido < metadata.longitud){
		int tamanio = 0;
		recibido += recv(metadata.socket,&tamanio,sizeof(int),0);
		buffer = malloc(tamanio);
		recibido += recv(metadata.socket,buffer,&tamanio,0);
		agregar_dato(mensaje,tamanio,buffer);
		free(buffer);
	}

	return mensaje;
}

MensajeDinamico* crear_mensaje_mdj_validar_archivo(int socket_destino, char* path){
	MensajeDinamico* mensaje_dinamico = crear_mensaje(VALIDAR_ARCHIVO,socket_destino);
	agregar_dato(mensaje_dinamico,strlen(path),path);
	return mensaje_dinamico;
}

MensajeDinamico* crear_mensaje_mdj_crear_archivo(int socket_destino, char* path, int cantidad_lineas){
	MensajeDinamico* mensaje_dinamico = crear_mensaje(CREAR_ARCHIVO,socket_destino);
	agregar_dato(mensaje_dinamico,sizeof(int),cantidad_lineas);
	agregar_dato(mensaje_dinamico,strlen(path),path);
	return mensaje_dinamico;
}

MensajeDinamico* crear_mensaje_mdj_obtener_datos(int socket_destino, char* path, int offset,int size){
	MensajeDinamico* mensaje_dinamico = crear_mensaje(OBTENER_DATOS,socket_destino);
	agregar_dato(mensaje_dinamico,sizeof(int),size);
	agregar_dato(mensaje_dinamico,sizeof(int),offset);
	agregar_dato(mensaje_dinamico,strlen(path),path);
	return mensaje_dinamico;
}

MensajeDinamico* crear_mensaje_mdj_guardar_datos(int socket_destino, char* path, int offset, int size, char* buffer){
	MensajeDinamico* mensaje_dinamico = crear_mensaje(OBTENER_DATOS,socket_destino);
	agregar_dato(mensaje_dinamico,strlen(buffer),buffer);
	agregar_dato(mensaje_dinamico,sizeof(int),size);
	agregar_dato(mensaje_dinamico,sizeof(int),offset);
	agregar_dato(mensaje_dinamico,strlen(path),path);
	return mensaje_dinamico;
}

