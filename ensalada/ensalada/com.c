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

void conectar_Servidor(int fd_socket, struct sockaddr_in *addr_servidor, Proceso t_proceso){
	int codigo = connect(fd_socket,(struct sockaddr*)addr_servidor, sizeof(struct sockaddr));
	comprobar_error(codigo, "Error al conectar a un servidor\0");

	handshakeCliente(t_proceso,fd_socket);
}

void comprobar_error(int variable, const char* mensajeError){
	if(variable < 0){
		perror(mensajeError);
		exit(1);
	}
}


void handshakeCliente(Proceso cliente, int socket_destino){
	int header = HANDSHAKE_CLIENTE;
	int error = send(socket_destino,&header,sizeof(int),0);
	comprobar_error(error,"Error en send\n");

	error = send(socket_destino,&cliente,sizeof(Proceso),0);
	comprobar_error(error,"Error en send\n");
}

Proceso handshakeServidor(int socket_origen){
	Proceso cliente = -1;
	int error = recv(socket_origen, &cliente, sizeof(Proceso),0);
	comprobar_error(error,"Error en recv\n");
	return cliente;
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
