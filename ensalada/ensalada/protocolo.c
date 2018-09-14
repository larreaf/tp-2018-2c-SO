#include "protocolo.h"

/*!
 * Recibe un stream de bytes y lo guarda como string
 * @param socket el socket a leer
 * @param linea array de char para almacenar el string, si no entra copia nada mas \0
 * @param tamanio_maximo_string int que indica el tamanio del array linea para no sobrepasarlo
 * @return el int que devuelve recv (cuantos bytes se recibieron o -1 para error
 */
int recibir_string(int socket, char* linea, int tamanio_maximo_string){
    char *buffer_string;
    int tamanio_string, ret;

    recv(socket, &tamanio_string, sizeof(int), 0);

    // Reservo memoria para la linea + 1 byte para el \0, luego recibo la linea y la imprimo
    buffer_string = malloc((size_t) tamanio_string + 1);
    ret = recv(socket, buffer_string, (size_t) tamanio_string, 0);
    buffer_string[tamanio_string] = '\0';

    // Verificar que el char* pasado tenga suficiente espacio para almacenar el string, sino segmentation fault
    if(tamanio_string<tamanio_maximo_string)
        strcpy(linea, buffer_string);
    else
        strcpy(linea, "\0");

    free(buffer_string);

    return ret;
}
