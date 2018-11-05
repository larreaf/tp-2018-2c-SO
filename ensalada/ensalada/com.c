#include "com.h"
#include "protocolo.h"


int crearSocket(){
	int fd_socket = socket(AF_INET,SOCK_STREAM,0);
	comprobar_error(fd_socket, "Error al inicializar un FD para un socket\0");
	return fd_socket;
}

void inicializarDireccion(struct sockaddr_in* addr_destino,int puerto_destino, char* ip_destino){
	addr_destino->sin_family = AF_INET;
	addr_destino->sin_port = htons((uint16_t)puerto_destino);
	addr_destino->sin_addr.s_addr = inet_addr(ip_destino);
	memset(addr_destino->sin_zero,'\0',8);
}

void asignarDireccion(int fd_socket, struct sockaddr_in* direccionSocket){
	int codigo = bind(fd_socket, (struct sockaddr*)direccionSocket, sizeof(struct sockaddr));
	comprobar_error(codigo, "Error en la funcion bind\0");
}

void reutilizarSocketEscucha(int fd_socket){
	int yes = 1;
	int codigo = setsockopt(fd_socket,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int));
	comprobar_error(codigo, "Error en la funcion setsockopt para reutilizar una direccion del socket\0");

}

int escuchar_Conexion( struct sockaddr_in* direccionLocal){
	int socket_escucha = 0;
	int codigo_error = 0;
	socket_escucha = crearSocket();
	reutilizarSocketEscucha(socket_escucha);
	asignarDireccion(socket_escucha, direccionLocal);
	codigo_error = listen(socket_escucha,10);
	comprobar_error(codigo_error, "Error en funcion listen\0");

	return socket_escucha; //Devuelve el socket que espera las nuevas conexiones entrantes

}
/*
void handshake(int socket_escucha,struct sockaddr_in direccionLocal){

}*/

int conectar_Servidor(int fd_socket, struct sockaddr_in *addr_servidor, Proceso t_proceso){
	int codigo = connect(fd_socket,(struct sockaddr*)addr_servidor, sizeof(struct sockaddr));
	comprobar_error(codigo, "Error al conectar a un servidor\0");

	return handshakeCliente(t_proceso,fd_socket);
}


int handshakeCliente(Proceso cliente, int socket_destino){
	MensajeDinamico* mensaje = crear_mensaje(HANDSHAKE_CLIENTE, socket_destino, 0);
	int header;

	agregar_dato(mensaje, sizeof(Proceso), &cliente);
	enviar_mensaje(mensaje);

	mensaje = recibir_mensaje(socket_destino);
	header = mensaje->header;
	destruir_mensaje(mensaje);

	if(header == HANDSHAKE_CLIENTE)
		return 0;
	else
		return -1;
}

int aceptar_conexion(int socketEscucha)
{
	int socketCliente;
	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);

	socketCliente = accept(socketEscucha, (struct sockaddr *) &addr, &addrlen);
	comprobar_error(socketCliente, "Error al aceptar la conexion\0");

	return socketCliente;
}
