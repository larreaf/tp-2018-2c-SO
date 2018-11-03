#include "mdj_functions.h"
#include "consola.h"

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


//TODO ver tamanio de linea
#define TAM_LINEA		35

pthread_t thread_consola;
t_bitarray* bitmap;
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
/*
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
        mensaje_saliente = crear_mensaje(STRING_CONSOLA_PROPIA, socket,0);

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
}*/

int main(int argc, char **argv) {

    int conexiones_permitidas[cantidad_tipos_procesos]={0}, header, retsocket=0;
    t_log* logger;
    ConexionesActivas conexiones_activas;

    MensajeDinamico* mensaje_recibido;
    MensajeDinamico* mensaje_respuesta;

    validar_parametros(argc);
    logger = log_create("mdj.log", "mdj", true, log_level_from_string("info"));
    configuracion = asignar_config(argv[1],mdj);
    log_info(logger, "Archivo de configuracion correcto");
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


  //  log_info(logger, "Listo");

    levantar_metadata(logger);

    log_info(logger, "Archivo metadata leido");

    /**
     * BITMAP con mmap
     */
    char* bitmap_file = string_new();
    char* bitarray;
    size_t SIZE_BITARRAY = (metadata.cantidad_bloques%8) ? (metadata.cantidad_bloques/8)+1 : metadata.cantidad_bloques/8;

    string_append_with_format(&bitmap_file,"%s/%s/%s",configuracion->punto_montaje,"Metadata","Bitmap.bin");
    int fd = open(bitmap_file,O_RDWR);
//	ftruncate(fd, SIZE_BITARRAY);

    bitarray = mmap(0, SIZE_BITARRAY, PROT_WRITE, MAP_SHARED, fd, 0);

    if(bitarray == MAP_FAILED){
        exit(5);
    }
    /*
     * El bitmap entregado para las pruebas intermedias es en MSB_FIRST
     */
    bitmap = bitarray_create_with_mode(bitarray,SIZE_BITARRAY,MSB_FIRST);
    log_info(logger, "Bitmap mapeado a memoria");

    //bitmap_clean();
    /*
     * Visualizar bitmap
     */
    char* bitmap_string = get_bitmap_to_string();
    log_info(logger,"Bitmap: %s",bitmap_string);
    free(bitmap_string);
    /*
    t_mdj_interface* mdj = malloc(sizeof(t_mdj_interface));
	mdj->path = string_new();
	string_append(&mdj->path,"premierLeague/equipo/arsenal");
	mdj->cantidad_lineas = 56;
	//crear_archivo(mdj);
	//borrar_archivo(mdj);*/

    pthread_create(&thread_consola, NULL, (void*)consola_mdj, NULL);
    pthread_detach(thread_consola);
    /**
     * WHILE
     */
    while (1){
        t_mdj_interface* data_operacion;

        //solo para obtener datos
        char* lineas_obtenidas;
        //solo para borrar_archivo

        // bloquea hasta recibir un MensajeEntrante y lo retorna, ademas internamente maneja handshakes y desconexiones
        // sin retornar
        mensaje_recibido = esperar_mensajes(conexiones_activas);
        /*
         * Retardo convertido a microsegundos
         */
        usleep(configuracion->retardo * 1000);
        // cuando esperar_mensajes retorna, devuelve un MensajeEntrante que tiene como campos el socket que lo envio,
        // el header que se envio y el tipo de proceso que lo envio

        switch (mensaje_recibido->header) {

            // en cada case del switch se puede manejar cada header como se desee
            case CREAR_ARCHIVO:
            	data_operacion = crear_data_mdj_operacion(mensaje_recibido);
            	log_info(logger, "Creando archivo %s...", data_operacion->path);
            	int cantidad_bytes_asignados = crear_archivo(data_operacion);
            	(cantidad_bytes_asignados > 0)?
            			log_info(logger, "Bytes asignados al archivo: %d",cantidad_bytes_asignados):
						log_error(logger,"Espacio insuficiente para crear el archivo");
            	mensaje_respuesta = crear_mensaje(CREAR_ARCHIVO,mensaje_recibido->socket, mensaje_recibido->particionado);
            	agregar_dato(mensaje_respuesta, sizeof(int) ,&cantidad_bytes_asignados );
            	log_info(logger,"Enviando respuesta...");
				enviar_mensaje(mensaje_respuesta);
				log_info(logger,"Respuesta enviada...");
                break;

            case BORRAR_ARCHIVO:
                data_operacion = crear_data_mdj_operacion(mensaje_recibido);
                log_info(logger, "Borrando archivo %s...", data_operacion->path);
                int borrar = borrar_archivo(data_operacion);
                (borrar == 0)?
                log_info(logger,"Archivo %s Borrado", data_operacion->path) :
                log_error(logger,"Error al intentar borrar el archivo %s, data_operacion->path");
                mensaje_respuesta = crear_mensaje(BORRAR_ARCHIVO,mensaje_recibido->socket, mensaje_recibido->particionado);
                agregar_dato(mensaje_respuesta,sizeof(int),&borrar);
                log_info(logger,"Enviando respuesta...");
                enviar_mensaje(mensaje_respuesta);
                log_info(logger,"Respuesta enviada...");
                break;

            case VALIDAR_ARCHIVO:
                data_operacion = crear_data_mdj_operacion(mensaje_recibido);
                log_info(logger, "Validando archivo %s...", data_operacion->path);
                bool respuesta = validar_archivo(data_operacion);
                respuesta ?
                log_info(logger, "El archivo %s existe ", data_operacion->path) :
                log_error(logger, "El archivo %s no existe ", data_operacion->path);
                mensaje_respuesta = crear_mensaje(VALIDAR_ARCHIVO,mensaje_recibido->socket, mensaje_recibido->particionado);
                agregar_dato(mensaje_respuesta,sizeof(bool),&respuesta);
                enviar_mensaje(mensaje_respuesta);
                break;

            case OBTENER_DATOS:
                log_info(logger, "Obteniendo data de operacion obtener datos...");
                data_operacion = crear_data_mdj_operacion(mensaje_recibido);
                log_info(logger, "Validando que el archivo exista");
                if(validar_archivo(data_operacion) == true){
                    /**
                     * El Archivo si existe
                     */
                    log_info(logger, "Obteniendo lineas de archivo %s...", data_operacion->path);
                    lineas_obtenidas = obtener_datos(data_operacion);
                    //printf("%s\n", lineas_obtenidas);
                    log_info(logger, "Enviando lineas...");
                    mensaje_respuesta = crear_mensaje(OBTENER_DATOS,mensaje_recibido->socket, mensaje_recibido->particionado);
                    agregar_string(mensaje_respuesta, lineas_obtenidas);
                    enviar_mensaje(mensaje_respuesta);
                    log_info(logger, "Lineas enviadas!");
                }else{
                    /**
                     * El Archivo no existe
                     */
                    log_error(logger, "El archivo %s no existe",data_operacion->path);
                    mensaje_respuesta = crear_mensaje(OBTENER_DATOS,mensaje_recibido->socket, mensaje_recibido->particionado);
                    int rta = 0;
                    agregar_dato(mensaje_respuesta,sizeof(int),&rta);
                    log_info(logger,"Enviando respuesta...");
                    enviar_mensaje(mensaje_respuesta);
                    log_info(logger,"Respuesta enviada...");
                }
                rl_restore_prompt();
                break;

            case GUARDAR_DATOS:
                log_info(logger, "Obteniendo data de operacion obtener datos...");
                data_operacion = crear_data_mdj_operacion(mensaje_recibido);
                log_info(logger, "Validando que el archivo exista");
                if(validar_archivo(data_operacion) == true){
                    /**
                     * El Archivo si existe
                     */
                    log_info(logger, "Guardando bytes en el archivo %s...", data_operacion->path);
                    int bytes_guardados = guardar_datos(data_operacion);
                    log_info(logger, "Se guardaron %d bytes", bytes_guardados);
                    mensaje_respuesta = crear_mensaje(GUARDAR_DATOS,mensaje_recibido->socket, mensaje_recibido->particionado);
                    agregar_dato(mensaje_respuesta, sizeof(int) ,&bytes_guardados);
                    log_info(logger,"Enviando respuesta...");
                    enviar_mensaje(mensaje_respuesta);
                    log_info(logger,"Respuesta enviada...");
                } else{
                    /**
                     * El Archivo no existe
                     */
                    log_error(logger, "El archivo %s no existe",data_operacion->path);
                    mensaje_respuesta = crear_mensaje(GUARDAR_DATOS,mensaje_recibido->socket, mensaje_recibido->particionado);
                    int rta = 0;
                    agregar_dato(mensaje_respuesta,sizeof(int),&rta);
                    log_info(logger,"Enviando respuesta...");
                    enviar_mensaje(mensaje_respuesta);
                    log_info(logger,"Respuesta enviada...");
                }
                break;

                /*case STRING_CONSOLA_PROPIA:
                    // este header indica que vamos a recibir un string leido de nuestra propia consola
    
                    // entonces recibimos el string y lo imprimimos, y ademas si es "exit" cerramos el programa
                    //str = recibir_string(mensaje_respuesta->socket);
                    comprobar_error(retsocket, "Error en recv de consola de MDJ");
                    printf("MDJ recibio de su propia consola: %s\n", str);
    
                    if(!strcmp(str, "exit")) {
                        free(str);
                      //  pthread_join(thread_consola, NULL);
                        cerrar_mdj(logger, configuracion, conexiones_activas);
                    }
                    free(str);
    
                    // TODO parsear string para interpretar comandos
                    // cuando terminamos de ejecutar lo que nos pidio la consola le mandamos un header
                    // OPERACION_CONSOLA_TERMINADA que le indica que terminamos en este caso
                    // tambien le podriamos mandar un mensaje cualquiera dependiendo de lo que nos haya pedido
                    header = OPERACION_CONSOLA_TERMINADA;
                    retsocket = send(mensaje_recibido->socket, &header, sizeof(OPERACION_CONSOLA_TERMINADA), 0);
                    comprobar_error(retsocket, "Error en send a consola de MDJ");
                    break;*/

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
       destruir_mensaje(mensaje_recibido);
    }
}



