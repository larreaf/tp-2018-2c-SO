//
// Created by utnso on 22/10/18.
//

#include "mensajes_mdj.h"


MensajeDinamico* crear_mensaje_mdj_validar_archivo(int socket_destino, char* path, int particion){
    MensajeDinamico* mensaje_dinamico = crear_mensaje(VALIDAR_ARCHIVO,socket_destino, particion);
    agregar_dato(mensaje_dinamico,strlen(path),path);
    return mensaje_dinamico;
}

MensajeDinamico* crear_mensaje_mdj_crear_archivo(int socket_destino, char* path, int cantidad_lineas, int particion){
    MensajeDinamico* mensaje_dinamico = crear_mensaje(CREAR_ARCHIVO,socket_destino, particion);
    agregar_string(mensaje_dinamico, path);
    agregar_dato(mensaje_dinamico,sizeof(int),&cantidad_lineas);
    return mensaje_dinamico;
}

MensajeDinamico* crear_mensaje_mdj_obtener_datos(int socket_destino, char* path, int offset,int size, int particion){
    MensajeDinamico* mensaje_dinamico = crear_mensaje(OBTENER_DATOS,socket_destino, particion);
    agregar_string(mensaje_dinamico, path);
    agregar_dato(mensaje_dinamico,sizeof(int),&offset);
    agregar_dato(mensaje_dinamico,sizeof(int),&size);
    return mensaje_dinamico;
}

MensajeDinamico* crear_mensaje_mdj_guardar_datos(int socket_destino, char* path, int offset, int size, char* buffer, int particion){
    MensajeDinamico* mensaje_dinamico = crear_mensaje(GUARDAR_DATOS,socket_destino, particion);
    agregar_string(mensaje_dinamico, path);
    agregar_dato(mensaje_dinamico,sizeof(int),&size);
    agregar_dato(mensaje_dinamico,sizeof(int),&offset);
    agregar_dato(mensaje_dinamico,strlen(buffer),buffer);
    return mensaje_dinamico;
}
MensajeDinamico* crear_mensaje_mdj_borrar_archivo(int socket_destino, char* path, int particion){
    MensajeDinamico* mensaje_dinamico = crear_mensaje(BORRAR_ARCHIVO,socket_destino, particion);
    agregar_dato(mensaje_dinamico,strlen(path),path);
    return mensaje_dinamico;
}
