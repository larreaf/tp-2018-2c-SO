#include <ensalada/protocolo.h>
#include <ensalada/servidor.h>
#include <ensalada/com.h>
#include <ensalada/mensaje.h>
#include "servidor_safa.h"
#include "types.h"
#include "plp.h"
#include "pcp.h"

pthread_t thread_consola, thread_plp, thread_pcp, thread_servidor;
sem_t cantidad_cpus, arrancar_planificadores;
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
    int err;

    validar_parametros(argc);
    configuracion = asignar_config(argv[1], safa);

    remove("safa.log");
    logger = log_create("safa.log", "S-AFA", (bool)configuracion->logger_consola,
            log_level_from_string(configuracion->logger_level));
    log_trace(logger, "Archivo de configuracion correcto");

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    err = sem_init(&cantidad_cpus, 0, 0);
    comprobar_error(err, "Error al iniciar semaforo cantidad_cpus");

    err = sem_init(&arrancar_planificadores, 0, 0);
    comprobar_error(err, "Error al iniciar semaforo arrancar_planificadores");

    // crear estructuras de planificadores
    plp = inicializar_plp(configuracion->multiprogramacion, configuracion->logger_level, configuracion->logger_consola);
    pcp = inicializar_pcp(configuracion->algoritmo, configuracion->quantum, configuracion->retardo,
            configuracion->logger_level, configuracion->logger_consola, configuracion->cant_lineas_equipo_grande);

    plp->cola_ready = pcp->cola_ready;
    plp->mutex_ready = pcp->mutex_ready;

    err = pthread_create(&thread_servidor, NULL, &ejecutar_servidor, NULL);
    comprobar_error(err, "Error al iniciar thread servidor");

    log_info(logger, "Esperando conexiones de elDiego y CPU...");
    sem_wait(&arrancar_planificadores);
    log_info(logger, "elDiego y CPU conectados, arrancando planificadores...");

    err = pthread_create(&thread_consola, NULL, &consola_safa, NULL);
    comprobar_error(err, "Error al iniciar thread consola");

    err = pthread_create(&thread_pcp, NULL, &ejecutar_pcp, pcp);
    comprobar_error(err, "Error al iniciar thread PCP");

    err = pthread_create(&thread_plp, NULL, &ejecutar_plp, plp);
    comprobar_error(err, "Error al iniciar thread PLP");

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
