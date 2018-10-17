#include <ensalada/protocolo.h>
#include <ensalada/servidor.h>
#include <ensalada/com.h>
#include <ensalada/mensaje.h>
#include "servidor_safa.h"
#include "types.h"
#include "plp.h"
#include "pcp.h"

pthread_t thread_consola, thread_plp, thread_pcp, thread_servidor;
sem_t cantidad_cpus;
t_log *logger;
ConexionesActivas conexiones_activas;
cfg_safa *configuracion;
PLP* plp;
PCP* pcp;
bool correr = 1;

void cerrar_safa(t_log* logger, cfg_safa* configuracion, ConexionesActivas server){
    log_info(logger, "Cerrando s-AFA...");

    // destruir_conexiones_activas manda headers CONEXION_CERRADA a todos los clientes conectados para que se enteren y despues
    // cierra cada socket
    destruir_conexiones_activas(server);
    log_destroy(logger);
    destroy_cfg(configuracion, t_safa);
    exit(0);
}

void sig_handler(int signo){
    if (signo == SIGTERM || signo == SIGKILL || signo == SIGINT) {
        cerrar_safa(logger, configuracion, conexiones_activas);
    }
}

int main(int argc, char **argv) {
    int conexiones_permitidas[cantidad_tipos_procesos] = {0}, retsocket=0, err, header;
    char* str;

    validar_parametros(argc);
    configuracion = asignar_config(argv[1], safa);

    remove("safa.log");
    logger = log_create("safa.log", "S-AFA", 1, LOG_LEVEL_INFO);
    log_trace(logger, "Archivo de configuracion correcto");

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    err = sem_init(&cantidad_cpus, 0, 0);
    comprobar_error(err, "Error al iniciar semaforo");

    // crear estructuras de planificadores
    plp = inicializar_plp(configuracion->multiprogramacion);
    pcp = inicializar_pcp(configuracion->algoritmo, configuracion->quantum);

    plp->cola_ready = pcp->cola_ready;
    plp->mutex_ready = pcp->mutex_ready;

    //TODO esperar a conexion de CPU y elDiego

    err = pthread_create(&thread_consola, NULL, &consola_safa, NULL);
    comprobar_error(err, "Error al iniciar thread consola");

    err = pthread_create(&thread_servidor, NULL, &ejecutar_servidor, NULL);
    comprobar_error(err, "Error al iniciar thread servidor");

    err = pthread_create(&thread_pcp, NULL, &ejecutar_pcp, pcp);
    comprobar_error(err, "Error al iniciar thread PCP");

    err = pthread_create(&thread_plp, NULL, &ejecutar_plp, plp);
    comprobar_error(err, "Error al iniciar thread PLP");

    sleep(1);
    log_info(logger, "Listo");

    // cerrar todos los hilos
    // TODO mover todo esto a cerrar_safa
    pthread_join(thread_consola, NULL);
    pthread_cancel(thread_plp);
    pthread_join(thread_plp, NULL);
    pthread_cancel(thread_pcp);
    pthread_join(thread_pcp, NULL);
    pthread_cancel(thread_servidor);
    pthread_join(thread_servidor, NULL);

    destruir_plp(plp);
    destruir_pcp(pcp);
    cerrar_safa(logger, configuracion, conexiones_activas);

    return 0;
}
