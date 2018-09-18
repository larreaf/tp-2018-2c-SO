#include "servidor.h"

/*!
 * conecta a otro servidor y devuelve el socket de esa conexion, tambien lo guarda en conexiones activas en el struct
 * Servidor
 * @param servidor servidor host que contiene la lista de conexiones activas
 * @param ip_destino ip del servidor al cual conectarse
 * @param puerto_destino puerto del servidor al cual conectarse
 * @param t_proceso_destino tipo de proceso destino
 * @return socket con la nueva conexion
 */
int conectar_como_cliente(Servidor servidor, char *ip_destino, int puerto_destino, Proceso t_proceso_destino){
    struct sockaddr_in addr;
    int socket = crearSocket();
    ConexionCliente* nueva_conexion;

    inicializarDireccion(&addr,puerto_destino,ip_destino);
    conectar_Servidor(socket,&addr, servidor.t_proceso_host);

    nueva_conexion = malloc(sizeof(ConexionCliente));
    nueva_conexion->socket = socket;
    nueva_conexion->t_proceso = t_proceso_destino;
    list_add(servidor.lista_clientes, nueva_conexion);

    return socket;
}

/*!
 * cierra socket cliente, primero enviando un header CONEXION_CERRADA, libera memoria y elimina el socket de la lista
 * de clientes del servidor
 * @param servidor servidor que contiene al socket cliente
 * @param posicion_cola posicion de la ConexionCliente en la lista de conexiones activas del servidor
 */
void cerrar_conexion(Servidor servidor, int posicion_cola){
    int header = CONEXION_CERRADA;
    ConexionCliente* cliente = list_get(servidor.lista_clientes, posicion_cola);

    send(cliente->socket, &header, sizeof(CONEXION_CERRADA), 0);
    close(cliente->socket);
    list_remove_and_destroy_element(servidor.lista_clientes, posicion_cola, free);
}

/*!
 * inicializa struct Servidor para multiplexar con select() y atender a varios clientes a la vez
 * @param logger logger a utilizar
 * @param puerto puerto en el cual crear el socket escucha
 * @param procesos_permitidos array de 4 int indicando que procesos se pueden conectar al servidor (o cuantos en caso de
 *        t_cpu
 * @param t_proceso_host tipo de proceso que ejecuta el servidor
 * @return struct Servidor con su logger, socket y lista de sockets clientes
 */
Servidor inicializar_servidor(t_log* logger, int puerto, int procesos_permitidos[6], Proceso t_proceso_host){
    int* procesos_conectados = calloc(sizeof(int), cantidad_tipos_procesos);
    int socket_escucha;
    struct sockaddr_in addr_local;
    Servidor servidor;

    inicializarDireccion(&addr_local, puerto, MY_IP);
    socket_escucha = escuchar_Conexion((&addr_local));

    servidor.inicializado = 1;
    servidor.logger = logger;
    servidor.socket = socket_escucha;
    servidor.lista_clientes = list_create();
    servidor.procesos_conectados = procesos_conectados;
    servidor.procesos_permitidos = procesos_permitidos;
    servidor.t_proceso_host = t_proceso_host;

    return servidor;
}

/*!
 * destruye el servidor, cerrando su socket de escucha, sockets de clientes si fuese necesario y liberando memoria
 * @param servidor struct Servidor a destruir
 */
void destruir_servidor(Servidor servidor){

    while(list_size(servidor.lista_clientes)){
        cerrar_conexion(servidor, 0);
    }

    close(servidor.socket);
    list_destroy(servidor.lista_clientes);
    free(servidor.procesos_conectados);
}

/*!
 * toma un struct Servidor, acepta conexiones nuevas realizando un handshake y retorna un MensajeEntrante que
 * contiene header, socket cliente del cual fue recibido, y tipo de proceso asociado a ese socket cliente.
 * bloquea hasta retornar un MensajeEntrante
 * @param servidor struct Servidor inicializada a monitorear
 * @return un MensajeEntrante con su campo header con el valor de header que se recibio, o -1 si hubo un error
 */
MensajeEntrante esperar_mensajes(Servidor servidor){
    ConexionCliente* cliente_seleccionado;
    fd_set descriptores_lectura;
    MensajeEntrante retorno;
    Proceso cliente;
    int header, retsel;

    if(servidor.inicializado != 1){
        log_error(servidor.logger, "Se intento utilizar un Servidor no inicializado");
        retorno.header = -1;
        return retorno;
    }

    while(1){
        // se mantiene una lista enlazada de struct ConexionCliente en el campo lista_clientes, los cuales contienen
        // el socket asociado a ese cliente y el tipo de proceso asociado a ese cliente (que se obtiene en el handshake)
        // el while se ejecuta solo si es una conexion nueva y un handshake, si no es asi se retorna un MensajeEntrante

        FD_ZERO(&descriptores_lectura);
        FD_SET(servidor.socket, &descriptores_lectura);

        for(int i = 0; i<list_size(servidor.lista_clientes); i++) {
            cliente_seleccionado = list_get(servidor.lista_clientes, i);
            FD_SET(cliente_seleccionado->socket, &descriptores_lectura);
        }
        log_trace(servidor.logger, "Esperando actividad en %d sockets clientes...", list_size(servidor.lista_clientes));
        retsel = select(FD_SETSIZE, &descriptores_lectura, NULL, NULL, NULL);

        if(retsel==-1){
            log_error(servidor.logger, "Error en select");
            retorno.header = -1;
            return retorno;
        }

        if (FD_ISSET(servidor.socket, &descriptores_lectura)) {
            // Hubo actividad en el socket de escucha, o sea hay alguien que se nos esta queriendo conectar

            log_info(servidor.logger, "Nueva conexion");
            cliente_seleccionado = malloc(sizeof(ConexionCliente));

            cliente_seleccionado->socket = aceptar_conexion(servidor.socket);
            list_add(servidor.lista_clientes, cliente_seleccionado);

            }else{

            for(int i = 0; i<list_size(servidor.lista_clientes); i++){
                cliente_seleccionado = list_get(servidor.lista_clientes, i);

                if(FD_ISSET(cliente_seleccionado->socket, &descriptores_lectura)){
                    if(recv(cliente_seleccionado->socket, &header, sizeof(int),0)>0){
                        if(header==HANDSHAKE_CLIENTE){
                            cliente = handshakeServidor(cliente_seleccionado->socket);

                            switch(cliente){
                                case t_cpu:
                                    cliente_seleccionado->t_proceso = t_cpu;
                                    if(servidor.procesos_conectados[t_cpu]<servidor.procesos_permitidos[t_cpu]){
                                        log_info(servidor.logger, "Handshake CPU realizado");
                                        servidor.procesos_conectados[t_cpu]++;
                                    }else{
                                        log_error(servidor.logger, "Conexion de proceso CPU denegada");
                                        cerrar_conexion(servidor, i);
                                    }
                                    break;

                                case t_elDiego:
                                    cliente_seleccionado->t_proceso = t_elDiego;

                                    if(servidor.procesos_permitidos[t_elDiego]==1){
                                        log_info(servidor.logger, "Handshake elDiego realizado");
                                        servidor.procesos_conectados[t_elDiego] = 1;
                                    }else{
                                        log_error(servidor.logger, "Conexion de proceso elDiego denegada");
                                        cerrar_conexion(servidor, i);
                                    }
                                    break;

                                case t_mdj:
                                    cliente_seleccionado->t_proceso = t_mdj;

                                    if(servidor.procesos_permitidos[t_mdj]==1){
                                        log_info(servidor.logger, "Handshake MDJ realizado");
                                        servidor.procesos_conectados[t_mdj] = 1;
                                    }else{
                                        log_error(servidor.logger, "Conexion de proceso MDJ denegada");
                                        cerrar_conexion(servidor, i);
                                    }
                                    break;

                                case t_fm9:
                                    cliente_seleccionado->t_proceso = t_fm9;

                                    if(servidor.procesos_permitidos[t_fm9]==1){
                                        log_info(servidor.logger, "Handshake FM9 realizado");
                                        servidor.procesos_conectados[t_fm9] = 1;
                                    }else{
                                        log_error(servidor.logger, "Conexion de proceso FM9 denegada");
                                        cerrar_conexion(servidor, i);
                                    }
                                    break;

                                case t_safa:
                                    cliente_seleccionado->t_proceso = t_safa;

                                    if(servidor.procesos_permitidos[t_safa]==1){
                                        log_info(servidor.logger, "Handshake S-AFA realizado");
                                        servidor.procesos_conectados[t_safa] = 1;
                                    }else{
                                        log_error(servidor.logger, "Conexion de proceso SAFA denegada");
                                        cerrar_conexion(servidor, i);
                                    }
                                    break;

                                case t_consola_mdj:
                                    cliente_seleccionado->t_proceso = t_consola_mdj;

                                    if(servidor.procesos_permitidos[t_consola_mdj]==1){
                                        log_info(servidor.logger, "Handshake consola MDJ realizado");
                                        servidor.procesos_conectados[t_consola_mdj] = 1;
                                    }else{
                                        log_error(servidor.logger, "Conexion de consola MDJ denegada");
                                        cerrar_conexion(servidor, i);
                                    }
                                    break;

                                case t_consola_fm9:
                                    cliente_seleccionado->t_proceso = t_consola_fm9;

                                    if(servidor.procesos_permitidos[t_consola_fm9]==1){
                                        log_info(servidor.logger, "Handshake consola FM9 realizado");
                                        servidor.procesos_conectados[t_consola_fm9] = 1;
                                    }else{
                                        log_error(servidor.logger, "Conexion de consola FM9 denegada");
                                        cerrar_conexion(servidor, i);
                                    }
                                    break;

                                case t_consola_safa:
                                    cliente_seleccionado->t_proceso = t_consola_safa;

                                    if(servidor.procesos_permitidos[t_consola_safa]==1){
                                        log_info(servidor.logger, "Handshake consola SAFA realizado");
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
                        }else{
                            retorno.header = header;
                            retorno.t_proceso = cliente_seleccionado->t_proceso;
                            retorno.socket = cliente_seleccionado->socket;
                            return retorno;
                        }
                    }else {
                        // Este switch se ejecuta cuando el recv retorna -1, lo cual por ahora significa que alguien
                        // se desconecto
                        switch (cliente_seleccionado->t_proceso) {
                            case t_cpu:
                                log_info(servidor.logger, "Proceso CPU desconectado (socket %d)",
                                        cliente_seleccionado->socket);
                                servidor.procesos_conectados[t_cpu]--;
                                break;
                            case t_elDiego:
                                log_info(servidor.logger, "Proceso elDiego desconectado (socket %d)",
                                        cliente_seleccionado->socket);
                                servidor.procesos_conectados[t_elDiego] = 0;
                                break;
                            case t_mdj:
                                log_info(servidor.logger, "Proceso MDJ desconectado (socket %d)",
                                        cliente_seleccionado->socket);
                                servidor.procesos_conectados[t_mdj] = 0;
                                break;
                            case t_safa:
                                log_info(servidor.logger, "Proceso S-AFA desconectado (socket %d)",
                                        cliente_seleccionado->socket);
                                servidor.procesos_conectados[t_safa] = 0;
                                break;
                            default:
                                log_warning(servidor.logger, "Proceso desconocido desconectado (socket %d)",
                                        cliente_seleccionado->socket);
                                break;
                        }
                        cerrar_conexion(servidor, i);
                    }
                }
            }
        }
    }
}
