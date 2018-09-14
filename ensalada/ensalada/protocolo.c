#include "protocolo.h"

/*!
 * Recibe un stream de bytes y lo guarda como string
 * @param socket el socket a leer
 * @param linea array de char para almacenar el string, si no entra copia nada mas \0
 * @param tamanio_maximo_string int que indica el tamanio del array linea para no sobrepasarlo
 * @return el int que devuelve recv (cuantos bytes se recibieron o -1 para error
 */
char* recibir_string(int socket){
    char *buffer_string;
    int tamanio_string, retrecv;

    retrecv = recv(socket, &tamanio_string, sizeof(int), 0);
    comprobar_error(retrecv, "Error en recv en recibir_string");

    // Reservo memoria para la linea + 1 byte para el \0, luego recibo la linea y la imprimo
    buffer_string = malloc((size_t) tamanio_string + 1);
    retrecv = recv(socket, buffer_string, (size_t) tamanio_string, 0);
    comprobar_error(retrecv, "Error en recv en recibir_string");
    buffer_string[tamanio_string] = '\0';

    return buffer_string;
}
