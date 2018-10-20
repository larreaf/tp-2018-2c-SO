#include "mdj_functions.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <commons/string.h>
#include <commons/log.h>
#include <commons/bitarray.h>
#include <readline/readline.h>
#include <ensalada/protocolo.h>
#include <ensalada/servidor.h>
#include <ensalada/mensaje.h>
#include <ensalada/validacion.h>



#define TAM_LINEA		35

pthread_t thread_consola;
t_bitarray* bitmap;
pthread_mutex_t mutex_bitmap;
cfg_mdj* configuracion;
metadata_fifa metadata;

void cerrar_mdj(t_log* logger, cfg_mdj* configuracion, ConexionesActivas conexiones_activas){
    log_info(logger, "Cerrando MDJ...");

    // destruir_conexiones_activas manda headers CONEXION_CERRADA a todos los clientes conectados para que se enteren y despues
    // cierra cada socket
    destruir_conexiones_activas(conexiones_activas);
    log_destroy(logger);
    destroy_cfg(configuracion, t_mdj);
    exit(0);
}

void* ejecutar_consola(void* arg){
    char *linea;
    MensajeDinamico* mensaje_saliente;
    int puerto = (int)arg;
    struct sockaddr_in addr;
    int socket = crearSocket(), header, retsocket = 0;

    inicializarDireccion(&addr,puerto,"127.0.0.1");
    conectar_Servidor(socket,&addr, t_consola_mdj);

    while(1) {
        // leemos linea y la mandamos al servidor (o sea al proceso MDJ porque somos la consola de MDJ)
        mensaje_saliente = crear_mensaje(STRING_CONSOLA_PROPIA, socket);

        linea = readline("> ");

        agregar_string(mensaje_saliente, linea);
        retsocket = enviar_mensaje(mensaje_saliente);
        comprobar_error(retsocket, "Error al enviar mensaje en hilo de consola de MDJ");

        // si la linea es "exit" dejamos de leer mas lineas y retornamos del thread
        if(!strcmp(linea, "exit")) {
            free(linea);
            return NULL;
        }
        free(linea);

        // aca nuestro servidor nos indica que termino de ejecutar lo que le pedimos, entonces volvemos al principio
        // del while
        retsocket = recv(socket, &header, sizeof(header), MSG_WAITALL);
        comprobar_error(retsocket, "Error en recv en hilo de consola de MDJ");
        if(header==OPERACION_CONSOLA_TERMINADA)
            continue;
    }
}

int main(int argc, char **argv) {
	pthread_mutex_init(&mutex_bitmap,NULL);

	char* str;
	int conexiones_permitidas[cantidad_tipos_procesos]={0}, err, header, retsocket=0;
	t_log* logger;
	ConexionesActivas conexiones_activas;

	MensajeDinamico* mensaje_respuesta;

    validar_parametros(argc);
    logger = log_create("mdj.log", "mdj", true, log_level_from_string("info"));
    configuracion = asignar_config(argv[1],mdj);
    if(configuracion->punto_montaje[0] == '/'){
    	configuracion->punto_montaje[0] = ' ';
		string_trim(&configuracion->punto_montaje);
    }
    if(configuracion->punto_montaje[string_length(configuracion->punto_montaje)-1] == '/'){
        	configuracion->punto_montaje[string_length(configuracion->punto_montaje)-1] = ' ';
    		string_trim(&configuracion->punto_montaje);
	}
	// conexiones_permitidas es un array de ints que indica que procesos se pueden conectar, o en el caso de t_cpu,
	// cuantas conexiones de cpu se van a aceptar
	// en este caso permitimos que se nos conecte el diego y nuestra propia consola
	conexiones_permitidas[t_elDiego] = 1;
	conexiones_permitidas[t_consola_mdj] = 1;
	conexiones_activas = inicializar_conexiones_activas(logger, configuracion->puerto, conexiones_permitidas, t_mdj);

    // intentamos arrancar el thread de la consola, que se va a bloquear hasta que esperemos mensajes mas abajo
    err = pthread_create(&thread_consola, NULL, &ejecutar_consola, (void*)configuracion->puerto);
    if (err != 0)
        printf("\ncan't create thread :[%s]", strerror(err));

    log_info(logger, "Listo");

    levantar_metadata();

    log_info(logger, "Archivo metadata leido");
    /**
     * BITMAP con mmap
     */
    char* bitmap_file = string_new();
    char* bitarray;
    off_t SIZE_BITARRAY = (metadata.cantidad_bloques%8) ? (metadata.cantidad_bloques/8)+1 : metadata.cantidad_bloques/8;

    string_append_with_format(&bitmap_file,"%s/%s/%s",configuracion->punto_montaje,"Metadata","Bitmap.bin");
    int fd = open(bitmap_file,O_RDWR);
    ftruncate(fd, SIZE_BITARRAY);

    bitarray = mmap(0,
    		SIZE_BITARRAY, PROT_WRITE, MAP_SHARED, fd, 0);

    if(bitarray == MAP_FAILED){
    	exit(5);
    }
    bitmap = bitarray_create_with_mode(bitarray,SIZE_BITARRAY,LSB_FIRST);
    log_info(logger, "Bitmap mapeado a memoria");

    t_mdj_interface* mdj = malloc(sizeof(t_mdj_interface));
	mdj->path = string_new();
	string_append(&mdj->path,"premierLeague/equipo/arsenal");
	mdj->cantidad_lineas = 56;
	//crear_archivo(mdj);
	borrar_archivo(mdj);

    /**
     * WHILE
     */
    while (1){

        // bloquea hasta recibir un MensajeEntrante y lo retorna, ademas internamente maneja handshakes y desconexiones
        // sin retornar
        mensaje_respuesta = esperar_mensajes(conexiones_activas);

        // cuando esperar_mensajes retorna, devuelve un MensajeEntrante que tiene como campos el socket que lo envio,
        // el header que se envio y el tipo de proceso que lo envio
        switch (mensaje_respuesta->header) {

            // en cada case del switch se puede manejar cada header como se desee
            case STRING_DIEGO_MDJ:
                // este header indica que el diego nos esta mandando un string

                // recibir_string recibe un stream de datos del socket del cual se envio el mensaje y los interpreta
                // como string, agregando \0 al final y metiendo los datos en el array str
               // str = recibir_string(mensaje_respuesta->socket);
                printf("MDJ recibio: %s\n", str);
                free(str);

                // para probar la capacidad de comunicacion bidireccional, le contestamos un "hola!"
                // el header STRING_MDJ_DIEGO significa que le estamos mandando un string al diego desde MDJ
                mensaje_respuesta = crear_mensaje(STRING_MDJ_DIEGO, mensaje_respuesta->socket);
                agregar_string(mensaje_respuesta, "Hola!");
                enviar_mensaje(mensaje_respuesta);

                break;

            case STRING_CONSOLA_PROPIA:
                // este header indica que vamos a recibir un string leido de nuestra propia consola

                // entonces recibimos el string y lo imprimimos, y ademas si es "exit" cerramos el programa
                //str = recibir_string(mensaje_respuesta->socket);
                comprobar_error(retsocket, "Error en recv de consola de MDJ");
                printf("MDJ recibio de su propia consola: %s\n", str);

                if(!strcmp(str, "exit")) {
                    free(str);
                    pthread_join(thread_consola, NULL);
                    cerrar_mdj(logger, configuracion, conexiones_activas);
                }
                free(str);

                // TODO parsear string para interpretar comandos
                // cuando terminamos de ejecutar lo que nos pidio la consola le mandamos un header
                // OPERACION_CONSOLA_TERMINADA que le indica que terminamos en este caso
                // tambien le podriamos mandar un mensaje cualquiera dependiendo de lo que nos haya pedido
                header = OPERACION_CONSOLA_TERMINADA;
                retsocket = send(mensaje_respuesta->socket, &header, sizeof(OPERACION_CONSOLA_TERMINADA), 0);
                comprobar_error(retsocket, "Error en send a consola de MDJ");
                break;

            case CONEXION_CERRADA:
                // el header CONEXION_CERRADA indica que el que nos envio ese mensaje se desconecto, idealmente los
                // procesos que cierran deberian mandar este header antes de hacerlo para que los procesos a los cuales
                // estan conectados se enteren, de todas maneras esperar_mensaje se encarga internamente de cerrar
                // el socket, liberar memoria, etc
                // cerrar_mdj(logger, configuracion, conexiones_activas);
                break;

            default:
                // TODO aca habria que programar que pasa si se manda un header invalido
                break;
        }
    }
}




