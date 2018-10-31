//
// Created by utnso on 22/10/18.
//

#include <ensalada/mensaje.h>

#ifndef ELDIEGO_MENSAJES_MDJ_H
#define ELDIEGO_MENSAJES_MDJ_H

/*
 * @NAME: crear_mensaje_mdj_validar_archivo
 * @DESC: crea el mensaje para enviar request a mdj para validar un archivo
 * @ARG: socket de mdj + path del archivo
 */
MensajeDinamico* crear_mensaje_mdj_validar_archivo(int socket_destino, char* path, int particion);

/*
 * @NAME: crear_mensaje_mdj_crear_archivo
 * @DESC: crea el mensaje para enviar request a mdj para crear un archivo
 * @ARG: socket de mdj + path del archivo
 */
MensajeDinamico* crear_mensaje_mdj_crear_archivo(int socket_destino, char* path, int cantidad_lineas, int particion);

/*
 * @NAME: crear_mensaje_mdj_obtener_datos
 * @DESC: crea el mensaje para enviar request a mdj para obtener datos
 * @ARG: socket de mdj
 * 		+ path del archivo
 * 		+ offset: desde donde leer
 *		+ size: tamanio a leer
 */
MensajeDinamico* crear_mensaje_mdj_obtener_datos(int socket_destino, char* path, int offset, int size, int particion);

/*
 * @NAME: crear_mensaje_mdj_guardar_datos
 * @DESC: crea el mensaje para enviar request a mdj para guardar datos
 * @ARG: socket de mdj
 * 		+ path del archivo
 * 		+ offset: desde donde leer
 *		+ size: tamanio a leer
 *		+ buffer: datos a guardar
 */
MensajeDinamico* crear_mensaje_mdj_guardar_datos(int socket_destino, char* path, int offset, int size, char* buffer, int particion);

/*
 * @NAME: crear_mensaje_mdj_borrar_archivo
 * @DESC: crea el mensaje para enviar request a mdj para borrar un archivo de mdj
 * @ARG: socket de mdj + path del archivo
 */
MensajeDinamico* crear_mensaje_mdj_borrar_archivo(int socket_destino, char* path, int particion);

#endif //ELDIEGO_MENSAJES_MDJ_H
