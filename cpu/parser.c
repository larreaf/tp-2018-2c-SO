#include "parser.h"

/*!
 * Toma un char* y lo parsea, retornando un Instruccion* con una lista de argumentos
 * @param linea char* a parsear
 * @return Instruccion* con la instruccion (opcode) y la lista de argumentos
 */
Instruccion* parsear_linea(char* linea){
    Instruccion* retorno = malloc(sizeof(Instruccion));
    int argc;
    char* word;

    retorno->argv = list_create();

    for(argc = 0; (word = strsep(&linea, " ")) != NULL; argc++){

        if(!argc) {
            retorno->opcode = str_a_opcode(word);
            continue;
        }
        list_add(retorno->argv, word);
    }
    retorno->argc = argc-1;
    return retorno;
}

/*!
 * Libera memoria de una instruccion
 * @param instruccion instruccion a destruir
 */
void destruir_instruccion(Instruccion* instruccion){
    list_destroy_and_destroy_elements(instruccion->argv, free);
    free(instruccion);
}

int validar_cant_argumentos(Instruccion* instruccion){
    if(instruccion->argc != cant_argumentos_instruccion[instruccion->opcode])
        return 0;
    else
        return -1;
}

/*!
 * Toma un char*, lo parsea y ejecuta la instruccion luego de esperar un retardo
 * @param dtb DTB* que se esta ejecutando
 * @param linea char* que se va a parsear y ejecutar
 * @param retardo tiempo a esperar antes de ejecutar la instruccion
 * @return READY si se puede ejecutar la proxima instruccion, BLOCK si se debe bloquear el DTB, DTB_EXIT si el DTB no
 * tiene mas instrucciones para ejecutar, un int codigo de error si hubo error, -1 si la cantidad de argumentos es
 * erronea, o -2 si no se reconoce la operacion
 */
int ejecutar_linea(DTB* dtb, char* linea, unsigned int retardo){
    Instruccion* instruccion = parsear_linea(linea);
    int resultado;
    long int string_to_int;

    resultado = validar_cant_argumentos(instruccion);
    if(resultado == -1)
        return resultado;

    sleep(retardo/1000);

    switch (instruccion->opcode){

        case OP_CREAR:
            string_to_int = strtol(list_get(instruccion->argv, 1), NULL, 10);
            resultado = in_crear(dtb->id, (char*)list_get(instruccion->argv, 0), (int)string_to_int);
            break;

        case OP_BORRAR:
            resultado = in_borrar(dtb->id, (char*)list_get(instruccion->argv, 0));
            break;

        case OP_ASIGNAR:
            string_to_int = strtol(list_get(instruccion->argv, 1), NULL, 10);
            resultado = in_asignar(dtb, (char*)list_get(instruccion->argv, 0), (int)string_to_int,
                                   (char*)list_get(instruccion->argv, 2));
            break;

        case OP_WAIT:
            break;

        case OP_SIGNAL:
            break;

        case OP_FLUSH:
            resultado = in_flush(dtb, (char*)list_get(instruccion->argv, 0));
            break;

        case OP_CONCENTRAR:
            resultado = in_concentrar();
            break;

        case OP_ABRIR:
            resultado = in_abrir(dtb, (char*)list_get(instruccion->argv, 0));
            break;

        case OP_CLOSE:
            resultado = in_close(dtb, (char*)list_get(instruccion->argv, 0));
            break;

        default:
            resultado = -2;
            break;
    }
    destruir_instruccion(instruccion);
    return resultado;
}

/*!
 * Convierte un char* que representa una operacion a un enum t_opcode
 * @param str char* a convertir
 * @return enum t_opcode correspondiente a la operacion
 */
t_opcode str_a_opcode(char* str){

    if(!strcmp(str, "abrir"))
        return OP_ABRIR;
    else if(!strcmp(str, "close"))
        return  OP_CLOSE;
    else if(!strcmp(str, "concentrar"))
        return  OP_CONCENTRAR;
    else if(!strcmp(str, "flush"))
        return  OP_FLUSH;
    else if(!strcmp(str, "signal"))
        return  OP_SIGNAL;
    else if(!strcmp(str, "wait"))
        return  OP_WAIT;
    else if(!strcmp(str, "asignar"))
        return  OP_ASIGNAR;
    else if(!strcmp(str, "crear"))
        return  OP_CREAR;
    else if(!strcmp(str, "borrar"))
        return  OP_BORRAR;
    else
        return -1;
}
