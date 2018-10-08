//
// Created by utnso on 30/09/18.
//

#include "instrucciones.h"

extern ConexionesActivas conexiones_activas;

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

    peticion_abrir = crear_mensaje(ABRIR_ARCHIVO_CPU_DIEGO, conexiones_activas.socket_eldiego);
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
    int error = 40001;

    int tamanio_lista = list_size(dtb->archivos_abiertos);

    for(int i = 0; i < tamanio_lista; i++) {
        if (!strcmp(path, list_get(dtb->archivos_abiertos, i)))
            error = 0;
    }

    if(error)
        return error;

    peticion_cerrar = crear_mensaje(CERRAR_ARCHIVO_CPU_FM9, conexiones_activas.socket_fm9);
    agregar_dato(peticion_cerrar, sizeof(int), &dtb->id);
    agregar_string(peticion_cerrar, path);
    enviar_mensaje(peticion_cerrar);
    // TODO verificar error en enviar_mensaje

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
    int error = 30001;

    int tamanio_lista = list_size(dtb->archivos_abiertos);

    for(int i = 0; i < tamanio_lista; i++) {
        if (!strcmp(path, list_get(dtb->archivos_abiertos, i)))
            error = 0;
    }

    if(error)
        return error;

    // TODO peticion flush a fm9

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

    peticion_crear = crear_mensaje(CREAR_ARCHIVO_CPU_DIEGO, conexiones_activas.socket_eldiego);
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

    peticion_borrar = crear_mensaje(BORRAR_ARCHIVO_CPU_DIEGO, conexiones_activas.socket_eldiego);
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
    int error = 20001;

    int tamanio_lista = list_size(dtb->archivos_abiertos);

    for(int i = 0; i < tamanio_lista; i++) {
        if (!strcmp(path, list_get(dtb->archivos_abiertos, i)))
            error = 0;
    }

    if(error)
        return error;

    peticion_asignar = crear_mensaje(ASIGNAR_ARCHIVO_CPU_FM9, conexiones_activas.socket_eldiego);
    agregar_dato(peticion_asignar, sizeof(int), &dtb->id);
    agregar_string(peticion_asignar, path);
    agregar_dato(peticion_asignar, sizeof(int), &linea);
    agregar_string(peticion_asignar, datos);
    enviar_mensaje(peticion_asignar);
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