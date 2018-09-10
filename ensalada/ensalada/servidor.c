#include "servidor.h"

/*!
 * inicializa struct Servidor para multiplexar con select() y atender a varios clientes a la vez
 * @param logger logger a utilizar
 * @param socket_escucha socket ya inicializado con listen
 * @return struct Servidor con su logger, socket y lista de sockets clientes
 */
Servidor inicializar_servidor(t_log* logger, int socket_escucha){
    int* procesos_conectados = calloc(sizeof(int), 4);
    Servidor servidor;

    servidor.inicializado = 1;
    servidor.logger = logger;
    servidor.socket = socket_escucha;
    servidor.lista_clientes = list_create();
    servidor.procesos_conectados = procesos_conectados;

    return servidor;
}

/*!
 * destruye el servidor, cerrando su socket de escucha, sockets de clientes si fuese necesario y liberando memoria
 * @param servidor struct Servidor a destruir
 */
void destruir_servidor(Servidor servidor){
    close(servidor.socket);
    list_destroy_and_destroy_elements(servidor.lista_clientes, free);
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
    int header;

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
        log_info(servidor.logger, "Esperando actividad en %d sockets clientes...", list_size(servidor.lista_clientes));
        select(FD_SETSIZE, &descriptores_lectura, NULL, NULL, NULL);

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
                                    log_info(servidor.logger, "Handshake CPU realizado");
                                    servidor.procesos_conectados[t_cpu]++;
                                    break;
                                case t_elDiego:
                                    cliente_seleccionado->t_proceso = t_elDiego;
                                    log_info(servidor.logger, "Handshake elDiego realizado");
                                    servidor.procesos_conectados[t_elDiego] = 1;
                                    break;
                                case t_mdj:
                                    cliente_seleccionado->t_proceso = t_mdj;
                                    log_info(servidor.logger, "Handshake MDJ realizado");
                                    servidor.procesos_conectados[t_mdj] = 1;
                                    break;
                                case t_safa:
                                    cliente_seleccionado->t_proceso = t_safa;
                                    log_info(servidor.logger, "Handshake S-AFA realizado");
                                    servidor.procesos_conectados[t_safa] = 1;
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
                                log_error(servidor.logger, "Proceso desconocido desconectado (socket %d)",
                                        cliente_seleccionado->socket);
                                break;
                        }
                        close(cliente_seleccionado->socket);
                        list_remove_and_destroy_element(servidor.lista_clientes, i, free);
                    }
                }
            }
        }
    }
}
