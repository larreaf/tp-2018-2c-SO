#include <ensalada/protocolo.h>
#include <ensalada/servidor.h>
#include <ensalada/com.h>
#include <ensalada/mensaje.h>
#include "types.h"

pthread_t thread_consola;

void cerrar_safa(t_log* logger, cfg_safa* configuracion, ConexionesActivas server){
    log_info(logger, "Cerrando s-AFA...");

    // destruir_conexiones_activas manda headers CONEXION_CERRADA a todos los clientes conectados para que se enteren y despues
    // cierra cada socket
    destruir_conexiones_activas(server);
    log_destroy(logger);
    destroy_cfg(configuracion, t_safa);
    exit(0);
}

void* ejecutar_consola(void* arg){
	char *linea;
	MensajeDinamico* mensaje_saliente;
	int puerto = (int)arg;
	struct sockaddr_in addr;
	int socket = crearSocket(), header, retsocket = 0;

	inicializarDireccion(&addr,puerto,MY_IP);
	conectar_Servidor(socket,&addr, t_consola_safa);

	while(1) {
		// leemos linea y la mandamos al servidor (o sea al proceso s-AFA porque somos la consola de s-AFA)
		mensaje_saliente = crear_mensaje(STRING_CONSOLA_PROPIA, socket);

		linea = readline("> ");

		agregar_string(mensaje_saliente, linea);
		retsocket = enviar_mensaje(mensaje_saliente);
		comprobar_error(retsocket, "Error al enviar mensaje en hilo de consola de s-AFA");

		// si la linea es "exit" dejamos de leer mas lineas y retornamos del thread
		if(!strcmp(linea, "exit")) {
			free(linea);
			return NULL;
		}
		free(linea);

		// aca nuestro servidor nos indica que termino de ejecutar lo que le pedimos, entonces volvemos al principio
		// del while
		retsocket = recv(socket, &header, sizeof(header), MSG_WAITALL);
		comprobar_error(retsocket, "Error en recv en hilo de consola de s-AFA");
		if(header==OPERACION_CONSOLA_TERMINADA)
			continue;
	}
}

int main(int argc, char **argv) {
    int conexiones_permitidas[cantidad_tipos_procesos] = {0}, retsocket=0, err, header;
    t_log *logger;
    cfg_safa *configuracion;
    ConexionesActivas conexiones_activas;
    MensajeEntrante mensaje;
    MensajeDinamico* mensaje_respuesta;
    char* str;

    validar_parametros(argc);
    configuracion = asignar_config(argv[1], safa);

    logger = log_create("safa.log", "S-AFA", 1, LOG_LEVEL_INFO);
    log_trace(logger, "Archivo de configuracion correcto");

    conexiones_permitidas[t_elDiego] = 1;
    conexiones_permitidas[t_consola_safa] = 1;
    conexiones_permitidas[t_cpu] = 1;
    conexiones_activas = inicializar_conexiones_activas(logger, configuracion->puerto, conexiones_permitidas, t_safa);

    // intentamos arrancar el thread de la consola, que se va a bloquear hasta que esperemos mensajes mas abajo
    err = pthread_create(&thread_consola, NULL, &ejecutar_consola, (void *) configuracion->puerto);
    if (err != 0)
        printf("\ncan't create thread :[%s]", strerror(err));

    log_info(logger, "Listo");

    while (1) {

        // bloquea hasta recibir un MensajeEntrante y lo retorna, ademas internamente maneja handshakes y desconexiones
        // sin retornar
        mensaje = esperar_mensajes(conexiones_activas);

        // cuando esperar_mensajes retorna, devuelve un MensajeEntrante que tiene como campos el socket que lo envio,
        // el header que se envio y el tipo de proceso que lo envio
        switch (mensaje.header) {

            // en cada case del switch se puede manejar cada header como se desee
            case STRING_DIEGO_SAFA:
                // este header indica que el diego nos esta mandando un string

                // recibir_string recibe un stream de datos del socket del cual se envio el mensaje y los interpreta
                // como string, agregando \0 al final y metiendo los datos en el array str
                str = recibir_string(mensaje.socket);
                printf("s-AFA recibio: %s\n", str);
                free(str);

                // para probar la capacidad de comunicacion bidireccional, le contestamos un "hola!"
                // el header STRING_MDJ_DIEGO significa que le estamos mandando un string al diego desde MDJ
                mensaje_respuesta = crear_mensaje(STRING_SAFA_DIEGO, mensaje.socket);
                agregar_string(mensaje_respuesta, "Hola!");
                enviar_mensaje(mensaje_respuesta);
                break;

            case STRING_CONSOLA_PROPIA:
                // este header indica que vamos a recibir un string leido de nuestra propia consola

                // entonces recibimos el string y lo imprimimos, y ademas si es "exit" cerramos el programa
                str = recibir_string(mensaje.socket);
                comprobar_error(retsocket, "Error en recv de consola de s-AFA");
                printf("s-AFA recibio de su propia consola: %s\n", str);

                if (!strcmp(str, "exit")) {
                    free(str);
                    pthread_join(thread_consola, NULL);
                    cerrar_safa(logger, configuracion, conexiones_activas);
                }
                free(str);

                // TODO parsear string para interpretar comandos
                // cuando terminamos de ejecutar lo que nos pidio la consola le mandamos un header
                // OPERACION_CONSOLA_TERMINADA que le indica que terminamos en este caso
                // tambien le podriamos mandar un mensaje cualquiera dependiendo de lo que nos haya pedido
                header = OPERACION_CONSOLA_TERMINADA;
                retsocket = send(mensaje.socket, &header, sizeof(OPERACION_CONSOLA_TERMINADA), 0);
                comprobar_error(retsocket, "Error en send a consola de s-AFA");
                break;

            case CONEXION_CERRADA:
                // el header CONEXION_CERRADA indica que el que nos envio ese mensaje se desconecto, idealmente los
                // procesos que cierran deberian mandar este header antes de hacerlo para que los procesos a los cuales
                // estan conectados se enteren, de todas maneras esperar_mensaje se encarga internamente de cerrar
                // su socket, liberar memoria, etc
                break;

            default:
                // TODO aca habria que programar que pasa si se manda un header invalido
                break;
        }
    }
}

/*

void sig_handler(int signo){
  if (signo == SIGTERM || signo == SIGKILL || signo == SIGINT) {
	  exit_gracefully();
  }
}



void exit_gracefully(){
	log_trace(logger,"Liberando Memoria");
	config_destroy(cfg_file);
	destroy_cfg(configuracion,safa);

	log_trace(logger,"Finalizo correctamente");
	log_destroy(logger);

	exit(EXIT_SUCCESS);
}
*/
