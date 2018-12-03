//
// Created by utnso on 29/11/18.
//

#include "config.h"

// El tamaño de un evento es igual al tamaño de la estructura de inotify
// mas el tamaño maximo de nombre de archivo que nosotros soportemos
// en este caso el tamaño de nombre maximo que vamos a manejar es de 24
// caracteres. Esto es porque la estructura inotify_event tiene un array
// sin dimension ( Ver C-Talks I - ANSI C ).
#define EVENT_SIZE  ( sizeof (struct inotify_event) + 24 )

// El tamaño del buffer es igual a la cantidad maxima de eventos simultaneos
// que quiero manejar por el tamaño de cada uno de los eventos. En este caso
// Puedo manejar hasta 1024 eventos simultaneos.
#define BUF_LEN     ( 1024 * EVENT_SIZE )

extern PCP* pcp;
extern PLP* plp;
extern t_log* logger;
extern int correr;
extern cfg_safa* configuracion;

void* monitorear_config(void *arg) {
    char buffer[BUF_LEN];
    char* filename = (char*)arg;
    struct inotify_event *event;
    int buffer_semaforo;

    // Al inicializar inotify este nos devuelve un descriptor de archivo
    int file_descriptor = inotify_init();
    if (file_descriptor < 0) {
        perror("inotify_init");
    }

    // Creamos un monitor sobre un path indicando que eventos queremos escuchar
    int watch_descriptor = inotify_add_watch(file_descriptor, "./safa.cfg", IN_MODIFY);

    log_info(logger, "Monitoreando config...");

    // El file descriptor creado por inotify, es el que recibe la información sobre los eventos ocurridos
    // para leer esta información el descriptor se lee como si fuera un archivo comun y corriente pero
    // la diferencia esta en que lo que leemos no es el contenido de un archivo sino la información
    // referente a los eventos ocurridos
    while(correr) {
        int length = read(file_descriptor, buffer, BUF_LEN);
        if (length < 0 || errno == EINTR) {
            perror("read");
            break;
        }

        int offset = 0;

        // Luego del read buffer es un array de n posiciones donde cada posición contiene
        // un eventos ( inotify_event ) junto con el nombre de este.
        while (offset < length) {

            // El buffer es de tipo array de char, o array de bytes. Esto es porque como los
            // nombres pueden tener nombres mas cortos que 24 caracteres el tamaño va a ser menor
            // a sizeof( struct inotify_event ) + 24.
            event = (struct inotify_event *) &buffer[offset];

            // El campo "len" nos indica la longitud del tamaño del nombre
            if (!event->len) {
                // Dentro de "mask" tenemos el evento que ocurrio y sobre donde ocurrio
                // sea un archivo o un directorio
                if (event->mask & IN_MODIFY) {
                    log_info(logger, "Actualizando config...");

                    sem_getvalue(&plp->semaforo_multiprogramacion, &buffer_semaforo);
                    buffer_semaforo = configuracion->multiprogramacion - buffer_semaforo;

                    configuracion = asignar_config(filename, safa);

                    //pthread_mutex_lock(&plp->mutex_pausa);
                    pthread_mutex_lock(&pcp->mutex_config);

                    pcp->algoritmo_planificacion = configuracion->algoritmo;
                    pcp->cantidad_lineas_equipo_grande = configuracion->cant_lineas_equipo_grande;
                    pcp->quantum = configuracion->quantum;
                    pcp->retardo_planificacion = configuracion->retardo;
                    memset(pcp->tiempos_respuesta, -1, TIEMPOS_RESPUESTA_SIZE);
                    pcp->cantidad_tiempos_respuesta = 0;

                    sem_destroy(&plp->semaforo_multiprogramacion);
                    sem_init(&plp->semaforo_multiprogramacion, 0, (unsigned int)configuracion->multiprogramacion);

                    for(int i = 0; i<buffer_semaforo; i++)
                        sem_wait(&plp->semaforo_multiprogramacion);

                    pthread_mutex_unlock(&pcp->mutex_config);
                    //pthread_mutex_unlock(&plp->mutex_pausa);

                    log_info(logger, "Config actualizada");

                }
            }
            offset += sizeof(struct inotify_event) + event->len;
        }
    }

    inotify_rm_watch(file_descriptor, watch_descriptor);
    close(file_descriptor);

    return NULL;
}
