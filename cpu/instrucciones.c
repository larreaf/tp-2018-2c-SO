//
// Created by utnso on 30/09/18.
//

#include "instrucciones.h"

extern ConexionesActivas conexiones_activas;
extern int socket_elDiego;
extern int socket_fm9;

/*!
 * Funcion que representa la operacion "abrir"
 * @param dtb DTB* que ejecuta la operacion
 * @param path char* con la ruta del archivo a abrir
 * @return BLOQUEAR si la solicitud a elDiego se envio correctamente, READY si el archivo ya esta abierto
 */
int in_abrir(DTB* dtb, char* path){
    MensajeDinamico* peticion_abrir;

    int tamanio_lista = list_size(dtb->archivos_abiertos);

    for(int i = 0; i < tamanio_lista; i++) {
        if (!strcmp(path, list_get(dtb->archivos_abiertos, i)))
            return READY;
    }

    peticion_abrir = crear_mensaje(ABRIR_ARCHIVO_CPU_DIEGO, socket_elDiego, 0);
    agregar_dato(peticion_abrir, sizeof(int), &dtb->id);
    agregar_string(peticion_abrir, path);
    enviar_mensaje(peticion_abrir);
    // TODO verificar error en enviar_mensaje

    return BLOQUEAR;
}

/*!
 * Funcion que representa la operacion "close"
 * @param dtb DTB* que ejecuta la operacion
 * @param path char* con la ruta del archivo a cerrar
 * @return READY si la solicitud a elDiego se envio correctamente, 40001 si el archivo no esta abierto
 */
int in_close(DTB* dtb, char* path){
    MensajeDinamico* peticion_cerrar;
    ArchivoAbierto* nodo_aux = NULL;
    int error = 40001;

    int tamanio_lista = list_size(dtb->archivos_abiertos);

    for(int i = 0; i < tamanio_lista; i++) {
        nodo_aux = list_get(dtb->archivos_abiertos, i);

        if (!strcmp(path, nodo_aux->path)) {
            error = 0;
            list_remove(dtb->archivos_abiertos, i);
            break;
        }
    }

    if(error)
        return error;

    peticion_cerrar = crear_mensaje(CERRAR_ARCHIVO_CPU_FM9, socket_fm9, 0);
    agregar_dato(peticion_cerrar, sizeof(int), &dtb->id);
    agregar_dato(peticion_cerrar, sizeof(int), &nodo_aux->direccion_memoria);
    enviar_mensaje(peticion_cerrar);
    // TODO verificar error en enviar_mensaje
    free(nodo_aux);

    return READY;
}

/*!
 * Funcion que representa la operacion "flush"
 * @param dtb DTB* que ejecuta la operacion
 * @param path char* con la ruta del archivo a flushear
 * @return BLOQUEAR si se envio la peticion a FM9, 30001 si el archivo no esta abierto
 */
int in_flush(DTB* dtb, char* path){
    MensajeDinamico* peticion_flush;
    ArchivoAbierto* nodo_aux = NULL;
    int error = 30001;

    int tamanio_lista = list_size(dtb->archivos_abiertos);

    for(int i = 0; i < tamanio_lista; i++) {
        nodo_aux = list_get(dtb->archivos_abiertos, i);

        if (!strcmp(path, nodo_aux->path)) {
            error = 0;
            break;
        }
    }

    if(error)
        return error;

    peticion_flush = crear_mensaje(FLUSH_ARCHIVO, socket_elDiego, 0);
    agregar_dato(peticion_flush, sizeof(int), &dtb->id);
    agregar_dato(peticion_flush, sizeof(int), &nodo_aux->direccion_memoria);
    agregar_string(peticion_flush, nodo_aux->path);
    enviar_mensaje(peticion_flush);

    return BLOQUEAR;
}

/*!
 * Funcion que representa la operacion "crear"
 * @param dtb_id id del DTB que solicita la operacion
 * @param path char* con la ruta del archivo a crear
 * @param cant_lineas cantidad de lineas del archivo a crear
 * @return BLOQUEAR si se envio la peticion correctamente a elDiego
 */
int in_crear(int dtb_id, char* path, int cant_lineas){
    MensajeDinamico* peticion_crear;

    peticion_crear = crear_mensaje(CREAR_ARCHIVO_CPU_DIEGO, socket_elDiego, 0);
    agregar_dato(peticion_crear, sizeof(int), &dtb_id);
    agregar_string(peticion_crear, path);
    agregar_dato(peticion_crear, sizeof(int), &cant_lineas);
    enviar_mensaje(peticion_crear);
    // TODO verificar error en enviar_mensaje

    return BLOQUEAR;
}

/*!
 * Funcion que representa la operacion "borrar"
 * @param dtb_id id del DTB que solicita la operacion
 * @param path char* con la ruta del archivo a borrar
 * @return BLOQUEAR si se envio la peticion correctamente a elDiego
 */
int in_borrar(int dtb_id, char* path){
    MensajeDinamico* peticion_borrar;

    peticion_borrar = crear_mensaje(BORRAR_ARCHIVO_CPU_DIEGO, conexiones_activas.socket_eldiego, 0);
    agregar_dato(peticion_borrar, sizeof(int), &dtb_id);
    agregar_string(peticion_borrar, path);
    enviar_mensaje(peticion_borrar);
    // TODO verificar error en enviar_mensaje

    return BLOQUEAR;
}

/*!
 * Funcion que representa la operacion "asignar"
 * @param dtb DTB* que solicita la operacion
 * @param path char* con la ruta del archivo a asignar
 * @param linea linea a editar
 * @param datos char* a escribir sobre la linea seleccionada
 * @return READY si se envio la peticion a FM9 correctamente, 20001 si el archivo no esta abierto
 */
int in_asignar(DTB* dtb, char* path, int linea, char* datos){
    MensajeDinamico* peticion_asignar;
    ArchivoAbierto* nodo_aux = NULL;
    int error = 20001, direccion, resultado;

    int tamanio_lista = list_size(dtb->archivos_abiertos);

    for(int i = 0; i < tamanio_lista && error; i++) {
        nodo_aux = list_get(dtb->archivos_abiertos, i);

        if (!strcmp(path, nodo_aux->path))
            error = 0;
    }

    if(error)
        return error;

    direccion = (nodo_aux->direccion_memoria)+(linea-1);

    peticion_asignar = crear_mensaje(ASIGNAR_ARCHIVO_CPU_FM9, socket_fm9, 0);
    agregar_dato(peticion_asignar, sizeof(int), &dtb->id);
    agregar_dato(peticion_asignar, sizeof(int), &direccion);
    agregar_string(peticion_asignar, datos);
    enviar_mensaje(peticion_asignar);

    peticion_asignar = recibir_mensaje(socket_fm9);
    recibir_int(&resultado, peticion_asignar);

    if(resultado)
        return resultado;

    // TODO verificar error en enviar_mensaje

    return READY;
}

int in_wait(){

    return 0;
}

int in_signal(){

    return 0;
}

/*!
 * Funcion que representa la operacion "concentrar"
 * @return READY
 */
int in_concentrar(){

    return READY;
}
