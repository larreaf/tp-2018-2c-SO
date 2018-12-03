//
// Created by utnso on 30/09/18.
//

#include "instrucciones.h"

extern ConexionesActivas conexiones_activas;
extern int socket_elDiego;
extern int socket_fm9;
extern int socket_safa;

/*!
 * Funcion que representa la operacion "abrir"
 * @param dtb DTB* que ejecuta la operacion
 * @param path char* con la ruta del archivo a abrir
 * @return BLOQUEAR si la solicitud a elDiego se envio correctamente, READY si el archivo ya esta abierto
 */
int in_abrir(DTB* dtb, char* path){
    MensajeDinamico* peticion_abrir, *consulta_archivo_abierto;
    int resultado;

    int tamanio_lista = list_size(dtb->archivos_abiertos);

    for(int i = 0; i < tamanio_lista; i++) {
        if (!strcmp(path, list_get(dtb->archivos_abiertos, i)))
            return READY;
    }
    consulta_archivo_abierto = crear_mensaje(CONSULTA_ARCHIVO_ABIERTO, socket_safa, 0);
    agregar_string(consulta_archivo_abierto, path);
    enviar_mensaje(consulta_archivo_abierto);

    consulta_archivo_abierto = recibir_mensaje(socket_safa);
    recibir_int(&resultado, consulta_archivo_abierto);
    destruir_mensaje(consulta_archivo_abierto);

    if(!resultado)
        return 10003;

    peticion_abrir = crear_mensaje(ABRIR_ARCHIVO_CPU_DIEGO, socket_elDiego, 0);
    agregar_int(peticion_abrir, dtb->id);
    agregar_string(peticion_abrir, path);

    if(enviar_mensaje(peticion_abrir)==-1)
        return -4;

    return BLOQUEAR;
}

/*!
 * Funcion que representa la operacion "close"
 * @param dtb DTB* que ejecuta la operacion
 * @param path char* con la ruta del archivo a cerrar
 * @return READY si la solicitud a elDiego se envio correctamente, 40001 si el archivo no esta abierto
 */
int in_close(DTB* dtb, char* path){
    MensajeDinamico* peticion_cerrar, *notificacion_safa;
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

    notificacion_safa = crear_mensaje(CERRAR_ARCHIVO_CPU_FM9, socket_safa, 0);
    agregar_string(notificacion_safa, path);
    if(enviar_mensaje(notificacion_safa)==-1)
        return -4;

    peticion_cerrar = crear_mensaje(CERRAR_ARCHIVO_CPU_FM9, socket_fm9, 0);
    agregar_int(peticion_cerrar, dtb->id);
    agregar_int(peticion_cerrar, nodo_aux->direccion_memoria);

    if(enviar_mensaje(peticion_cerrar)==-1) {
        free(nodo_aux->path);
        free(nodo_aux);
        return -4;
    }
    free(nodo_aux->path);
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
    agregar_int(peticion_flush, dtb->id);
    agregar_int(peticion_flush, nodo_aux->direccion_memoria);
    agregar_string(peticion_flush, nodo_aux->path);
    if(enviar_mensaje(peticion_flush)==-1)
        return -4;

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
    agregar_int(peticion_crear, dtb_id);
    agregar_string(peticion_crear, path);
    agregar_int(peticion_crear, cant_lineas);
    if(enviar_mensaje(peticion_crear)==-1)
        return -4;

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

    peticion_borrar = crear_mensaje(BORRAR_ARCHIVO_CPU_DIEGO, socket_elDiego, 0);
    agregar_int(peticion_borrar, dtb_id);
    agregar_string(peticion_borrar, path);
    if(enviar_mensaje(peticion_borrar)==-1)
        return -4;

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
    agregar_int(peticion_asignar, dtb->id);
    agregar_int(peticion_asignar, direccion);
    agregar_string(peticion_asignar, datos);
    if(enviar_mensaje(peticion_asignar)==-1)
        return -4;

    peticion_asignar = recibir_mensaje(socket_fm9);
    recibir_int(&resultado, peticion_asignar);
    destruir_mensaje(peticion_asignar);

    if(resultado)
        return resultado;

    return READY;
}

/*!
 * Funcion que representa la instruccion "wait"
 * @param dtb
 * @param nombre_recurso
 * @return
 */
int in_wait(DTB* dtb, char* nombre_recurso){
    MensajeDinamico* peticion_wait;
    int resultado;

    peticion_wait = crear_mensaje(SOLICITUD_RECURSO, socket_safa, 0);
    agregar_int(peticion_wait, dtb->id);
    agregar_string(peticion_wait, nombre_recurso);
    if(enviar_mensaje(peticion_wait)==-1)
        return -4;

    peticion_wait = recibir_mensaje(socket_safa);
    recibir_int(&resultado, peticion_wait);
    destruir_mensaje(peticion_wait);

    if(resultado)
        return READY;
    else
        return BLOQUEAR;
}

/*!
 * Funcion que representa la operacion "signal"
 * @param nombre_recurso
 * @return
 */
int in_signal(char* nombre_recurso){
    MensajeDinamico* peticion_signal;

    peticion_signal = crear_mensaje(LIBERAR_RECURSO, socket_safa, 0);
    agregar_string(peticion_signal, nombre_recurso);
    if(enviar_mensaje(peticion_signal)==-1)
        return -4;

    return READY;
}

/*!
 * Funcion que representa la operacion "concentrar"
 * @return READY
 */
int in_concentrar(){
    return READY;
}
