#include "servidor.h"

/*!
 * conecta a otro servidor y devuelve el socket de esa conexion, tambien lo guarda en conexiones activas en el struct
 * Servidor
 * @param conexiones_activas servidor host que contiene la lista de conexiones activas
 * @param ip_destino ip del servidor al cual conectarse
 * @param puerto_destino puerto del servidor al cual conectarse
 * @param t_proceso_destino tipo de proceso destino
 * @return socket con la nueva conexion
 */
int conectar_como_cliente(ConexionesActivas conexiones_activas, char *ip_destino, int puerto_destino, Proceso t_proceso_destino){
    struct sockaddr_in addr;
    int socket = crearSocket();
    ConexionCliente* nueva_conexion;

    inicializarDireccion(&addr,puerto_destino,ip_destino);
    conectar_Servidor(socket,&addr, conexiones_activas.t_proceso_host);

    nueva_conexion = malloc(sizeof(ConexionCliente));
    nueva_conexion->socket = socket;
    nueva_conexion->t_proceso = t_proceso_destino;
    list_add(conexiones_activas.lista_clientes, nueva_conexion);

    return socket;
}

/*!
 * cierra socket cliente, primero enviando un header CONEXION_CERRADA, libera memoria y elimina el socket de la lista
 * de clientes del servidor
 * @param conexiones_activas servidor que contiene al socket cliente
 * @param socket numero de socket a cerrar
 */
void cerrar_conexion(ConexionesActivas conexiones_activas, int socket){
    ConexionCliente* cliente_seleccionado;
    CPU* cpu_seleccionado;

    // TODO optimizar

    for(int i = 0; i<list_size(conexiones_activas.lista_clientes); i++) {

        cliente_seleccionado = list_get(conexiones_activas.lista_clientes, i);
        if(cliente_seleccionado->socket == socket){
            //mensaje = crear_mensaje(CONEXION_CERRADA, socket, 0);
            //enviar_mensaje(mensaje);

            if(cliente_seleccionado->t_proceso == t_cpu){

                for(int j = 0; j<list_size(conexiones_activas.lista_cpus); j++){
                    cpu_seleccionado = list_get(conexiones_activas.lista_cpus, j);

                    if(cpu_seleccionado->socket == socket)
                        list_remove_and_destroy_element(conexiones_activas.lista_cpus, j, free);
                }
            }
            close(cliente_seleccionado->socket);
            list_remove_and_destroy_element(conexiones_activas.lista_clientes, i, free);
        }
    }
}

/*!
 * inicializa struct Servidor para multiplexar con select() y atender a varios clientes a la vez
 * @param logger logger a utilizar
 * @param puerto puerto en el cual crear el socket escucha, si es 0 no se crea puerto de escucha
 * @param procesos_permitidos array de 4 int indicando que procesos se pueden conectar al servidor (o cuantos en caso de
 *        t_cpu
 * @param t_proceso_host tipo de proceso que ejecuta el servidor
 * @return struct Servidor con su logger, socket y lista de sockets clientes
 */
ConexionesActivas inicializar_conexiones_activas(t_log *logger, int puerto, int *procesos_permitidos,
                                                 Proceso t_proceso_host){
    int* procesos_conectados = calloc(sizeof(int), cantidad_tipos_procesos);
    int socket_escucha = 0;
    struct sockaddr_in addr_local;
    ConexionesActivas servidor;

    if(puerto){
        inicializarDireccion(&addr_local, puerto, MY_IP);
        socket_escucha = escuchar_Conexion((&addr_local));
    }

    servidor.inicializado = 1;
    servidor.logger = logger;
    servidor.socket = socket_escucha;
    servidor.lista_clientes = list_create();
    servidor.lista_cpus = list_create();
    servidor.procesos_conectados = procesos_conectados;
    servidor.procesos_permitidos = procesos_permitidos;
    servidor.t_proceso_host = t_proceso_host;
    servidor.socket_eldiego - -1;
    servidor.socket_fm9 = -1;
    servidor.socket_safa = -1;
    servidor.socket_mdj = -1;
    sem_init(&(servidor.semaforo_safa), 0, 0);

    return servidor;
}

/*!
 * destruye el servidor, cerrando su socket de escucha, sockets de clientes si fuese necesario y liberando memoria
 * @param servidor struct Servidor a destruir
 */
void destruir_conexiones_activas(ConexionesActivas servidor){
    ConexionCliente* cliente_seleccionado;

    while(list_size(servidor.lista_clientes)){
        cliente_seleccionado = list_get(servidor.lista_clientes, 0);
        cerrar_conexion(servidor, cliente_seleccionado->socket);
    }

    close(servidor.socket);
    sem_destroy(&(servidor.semaforo_safa));
    list_destroy(servidor.lista_clientes);
    list_destroy(servidor.lista_cpus);
    free(servidor.procesos_conectados);
}

/*!
 * toma un struct Servidor, acepta conexiones nuevas realizando un handshake y retorna un MensajeEntrante que
 * contiene header, socket cliente del cual fue recibido, y tipo de proceso asociado a ese socket cliente.
 * bloquea hasta retornar un MensajeEntrante
 * @param servidor struct Servidor inicializada a monitorear
 * @return un MensajeEntrante con su campo header con el valor de header que se recibio, o -1 si hubo un error
 */
MensajeDinamico* esperar_mensajes(ConexionesActivas servidor){
    ConexionCliente* cliente_seleccionado;
    fd_set descriptores_lectura;
    MensajeDinamico* retorno;
    Proceso cliente;
    CPU* cpu;
    int retsel, ids_cpu = 0;

    if(servidor.inicializado != 1){
        log_error(servidor.logger, "Se intento utilizar un ConexionesActivas no inicializado");
        retorno = crear_mensaje(-1, 0, 0);
        retorno->header = -1;
        return retorno;
    }

    while(1){
        // se mantiene una lista enlazada de struct ConexionCliente en el campo lista_clientes, los cuales contienen
        // el socket asociado a ese cliente y el tipo de proceso asociado a ese cliente (que se obtiene en el handshake)
        // el while se ejecuta solo si es una conexion nueva y un handshake, si no es asi se retorna un MensajeEntrante

        FD_ZERO(&descriptores_lectura);

        if(servidor.socket)
            FD_SET(servidor.socket, &descriptores_lectura);

        if(servidor.procesos_conectados[t_elDiego] && servidor.procesos_conectados[t_cpu])
            sem_post(&(servidor.semaforo_safa));

        for(int i = 0; i<list_size(servidor.lista_clientes); i++) {
            cliente_seleccionado = list_get(servidor.lista_clientes, i);
            FD_SET(cliente_seleccionado->socket, &descriptores_lectura);
        }
        log_trace(servidor.logger, "Esperando actividad en %d sockets clientes...", list_size(servidor.lista_clientes));
        retsel = select(FD_SETSIZE, &descriptores_lectura, NULL, NULL, NULL);

        if(retsel==-1){
            log_error(servidor.logger, "Error en select");
            retorno = crear_mensaje(-1, 0, 0);
            retorno->header = -1;
            return retorno;
        }

        if (servidor.socket && FD_ISSET(servidor.socket, &descriptores_lectura)) {
            // Hubo actividad en el socket de escucha, o sea hay alguien que se nos esta queriendo conectar

            log_trace(servidor.logger, "Nueva conexion");
            cliente_seleccionado = malloc(sizeof(ConexionCliente));

            cliente_seleccionado->socket = aceptar_conexion(servidor.socket);
            list_add(servidor.lista_clientes, cliente_seleccionado);

            }else{

            for(int i = 0; i<list_size(servidor.lista_clientes); i++){
                cliente_seleccionado = list_get(servidor.lista_clientes, i);

                if(FD_ISSET(cliente_seleccionado->socket, &descriptores_lectura)){
                    retorno = recibir_mensaje(cliente_seleccionado->socket);

                    if(retorno->header!=-1){
                        if(retorno->header==HANDSHAKE_CLIENTE){
                            recibir_int((int*)&cliente, retorno);

                            switch(cliente){
                                case t_cpu:
                                    cliente_seleccionado->t_proceso = t_cpu;
                                    if(servidor.procesos_conectados[t_cpu]<servidor.procesos_permitidos[t_cpu]){
                                        log_info(servidor.logger, "Proceso CPU conectado (socket %d)",
                                                cliente_seleccionado->socket);
                                        servidor.procesos_conectados[t_cpu]++;

                                        cpu = malloc(sizeof(CPU));
                                        cpu->socket = cliente_seleccionado->socket;
                                        cpu->id = ids_cpu++;
                                        cpu->cantidad_procesos_asignados = 0;
                                        list_add(servidor.lista_cpus, cpu);

                                    }else{
                                        log_error(servidor.logger, "Conexion de proceso CPU denegada");
                                        cerrar_conexion(servidor, i);
                                    }
                                    break;

                                case t_elDiego:
                                    cliente_seleccionado->t_proceso = t_elDiego;

                                    if(servidor.procesos_permitidos[t_elDiego]==1){
                                        log_info(servidor.logger, "Proceso elDiego conectado (socket %d)",
                                                 cliente_seleccionado->socket);
                                        servidor.procesos_conectados[t_elDiego] = 1;
                                        servidor.socket_eldiego = cliente_seleccionado->socket;
                                    }else{
                                        log_error(servidor.logger, "Conexion de proceso elDiego denegada");
                                        cerrar_conexion(servidor, i);
                                    }
                                    break;

                                case t_mdj:
                                    cliente_seleccionado->t_proceso = t_mdj;

                                    if(servidor.procesos_permitidos[t_mdj]==1){
                                        log_info(servidor.logger, "Proceso MDJ conectado (socket %d)",
                                                 cliente_seleccionado->socket);
                                        servidor.procesos_conectados[t_mdj] = 1;
                                        servidor.socket_mdj = cliente_seleccionado->socket;
                                    }else{
                                        log_error(servidor.logger, "Conexion de proceso MDJ denegada");
                                        cerrar_conexion(servidor, i);
                                    }
                                    break;

                                case t_fm9:
                                    cliente_seleccionado->t_proceso = t_fm9;

                                    if(servidor.procesos_permitidos[t_fm9]==1){
                                        log_info(servidor.logger, "Proceso FM9 conectado (socket %d)",
                                                 cliente_seleccionado->socket);
                                        servidor.procesos_conectados[t_fm9] = 1;
                                        servidor.socket_fm9 = cliente_seleccionado->socket;
                                    }else{
                                        log_error(servidor.logger, "Conexion de proceso FM9 denegada");
                                        cerrar_conexion(servidor, i);
                                    }
                                    break;

                                case t_safa:
                                    cliente_seleccionado->t_proceso = t_safa;

                                    if(servidor.procesos_permitidos[t_safa]==1){
                                        log_info(servidor.logger, "Proceso SAFA conectado (socket %d)",
                                                 cliente_seleccionado->socket);
                                        servidor.procesos_conectados[t_safa] = 1;
                                        servidor.socket_safa = cliente_seleccionado->socket;
                                    }else{
                                        log_error(servidor.logger, "Conexion de proceso SAFA denegada");
                                        cerrar_conexion(servidor, i);
                                    }
                                    break;

                                case t_consola_mdj:
                                    cliente_seleccionado->t_proceso = t_consola_mdj;

                                    if(servidor.procesos_permitidos[t_consola_mdj]==1){
                                        log_info(servidor.logger, "Hilo consola MDJ conectado (socket %d)",
                                                 cliente_seleccionado->socket);
                                        servidor.procesos_conectados[t_consola_mdj] = 1;
                                    }else{
                                        log_error(servidor.logger, "Conexion de consola MDJ denegada");
                                        cerrar_conexion(servidor, i);
                                    }
                                    break;

                                case t_consola_fm9:
                                    cliente_seleccionado->t_proceso = t_consola_fm9;

                                    if(servidor.procesos_permitidos[t_consola_fm9]==1){
                                        log_info(servidor.logger, "Hilo consola FM9 conectado (socket %d)",
                                                 cliente_seleccionado->socket);
                                        servidor.procesos_conectados[t_consola_fm9] = 1;
                                    }else{
                                        log_error(servidor.logger, "Conexion de consola FM9 denegada");
                                        cerrar_conexion(servidor, i);
                                    }
                                    break;

                                case t_consola_safa:
                                    cliente_seleccionado->t_proceso = t_consola_safa;

                                    if(servidor.procesos_permitidos[t_consola_safa]==1){
                                        log_info(servidor.logger, "Hilo consola SAFA conectado (socket %d)",
                                                 cliente_seleccionado->socket);
                                        servidor.procesos_conectados[t_consola_safa] = 1;
                                    }else{
                                        log_error(servidor.logger, "Conexion de consola SAFA denegada");
                                        cerrar_conexion(servidor, i);
                                    }
                                    break;

                                default:
                                    log_error(servidor.logger, "Falla handshake, tipo de proceso invalido");
                                    break;
                            }
                            retorno->header = NUEVA_CONEXION;
                            retorno->t_proceso = cliente;
                            return retorno;
                        }else{
                            retorno->t_proceso = cliente_seleccionado->t_proceso;
                            return retorno;
                        }
                    }else {
                        // Este switch se ejecuta cuando el mensaje tiene header -1, lo cual por ahora significa que alguien
                        // se desconecto
                        switch (cliente_seleccionado->t_proceso) {
                            case t_cpu:
                                log_warning(servidor.logger, "Proceso CPU desconectado (socket %d)",
                                        cliente_seleccionado->socket);
                                servidor.procesos_conectados[t_cpu]--;
                                break;
                            case t_elDiego:
                                log_warning(servidor.logger, "Proceso elDiego desconectado (socket %d)",
                                        cliente_seleccionado->socket);
                                servidor.procesos_conectados[t_elDiego] = 0;
                                servidor.socket_eldiego = -1;
                                break;
                            case t_mdj:
                                log_warning(servidor.logger, "Proceso MDJ desconectado (socket %d)",
                                        cliente_seleccionado->socket);
                                servidor.procesos_conectados[t_mdj] = 0;
                                servidor.socket_mdj = -1;
                                break;
                            case t_safa:
                                log_warning(servidor.logger, "Proceso S-AFA desconectado (socket %d)",
                                        cliente_seleccionado->socket);
                                servidor.procesos_conectados[t_safa] = 0;
                                servidor.socket_safa = -1;
                                break;
                            case t_fm9:
                                log_warning(servidor.logger, "Proceso FM9 desconectado (socket %d)",
                                         cliente_seleccionado->socket);
                                servidor.procesos_conectados[t_fm9] = 0;
                                servidor.socket_fm9 = -1;
                                break;
                            default:
                                log_warning(servidor.logger, "Proceso desconocido desconectado (socket %d)",
                                        cliente_seleccionado->socket);
                                break;
                        }
                        cerrar_conexion(servidor, cliente_seleccionado->socket);
                        retorno = crear_mensaje(CONEXION_CERRADA, cliente_seleccionado->socket, 0);
                        retorno->t_proceso = cliente_seleccionado->t_proceso;
                        return retorno;
                    }
                }
            }
        }
    }
}
