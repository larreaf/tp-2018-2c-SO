#ifndef REDISTINTO_COM_H_
#define REDISTINTO_COM_H_

#include <stdlib.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <commons/log.h>
#include "mensaje.h"


#define MY_IP "127.0.0.1"


/*
 * @NAME: crearSocket
 * @DESC: retorna un file descriptor para un nuevo socket.
 */
int crearSocket();

/*
 * @NAME: inicializarDireccion
 * @DESC: inicializa una estructura pasada por parametro con los subsiguientes datos.
 */
void inicializarDireccion(struct sockaddr_in* addr_destino,int puerto_destino, char* ip_destino);

/*
 * @NAME: asignarDireccion
 * @DESC: Le asigna una direccion al socket con bind.
 */
void asignarDireccion(int fd_socket,struct sockaddr_in* );

/*
 * @NAME: reutilizarSocketEscucha
 * @DESC: el socket pasa a ser reutilizable. Simil setsockopt.
 */
void reutilizarSocketEscucha(int );

/*
 * @NAME: escuchar_Conexion
 * @DESC: crea un socket y lo pone a escuchar (con listen) utilizando la estructura (que contiene la ip y el puerto)
 * que se le pasa por parametro.
 * @RET: devuelve el socket que se encuentra escuchando conexiones.
 */
int escuchar_Conexion( struct sockaddr_in*);

/*
 * @NAME: conectar_Servidor
 * @DESC: conecta el socket al servidor con la direccion addr_servidor.
 */
int conectar_Servidor(int fd_socket, struct sockaddr_in *addr_servidor, Proceso t_proceso);

/*
 * @NAME: aceptar_conexion
 * @DESC: acepta la conexion proveniente del socket pasado por parametro.
 */
int aceptar_conexion(int socketEscucha);

/*
 * @NAME: handshakeCliente
 * @DESC: Envia que tipo de cliente es hacia el servidor al que quiere conectarse
 *
 */
int handshakeCliente(Proceso cliente, int socket_destino);

/*
 * @NAME: handshakeCliente
 * @DESC: Recibe en cliente el tipo de cliente.
 */






#endif /* REDISTINTO_COM_H_ */
