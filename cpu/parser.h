//
// Created by utnso on 29/09/18.
//

#ifndef CPU_PARSER_H
#define CPU_PARSER_H

#include <stdlib.h>
#include <string.h>
#include <commons/collections/list.h>
#include <ensalada/dtb.h>
#include "instrucciones.h"

typedef enum {
    ARGC_ABRIR = 1,
    ARGC_ASIGNAR = 3,
    ARGC_CONCENTRAR = 0,
    ARGC_WAIT = 1,
    ARGC_SIGNAL = 1,
    ARGC_FLUSH = 1,
    ARGC_CLOSE = 1,
    ARGC_CREAR = 2,
    ARGC_BORRAR = 1,
}cant_arg_instruccion;

typedef enum{
    OP_ABRIR,
    OP_CLOSE,
    OP_CONCENTRAR,
    OP_ASIGNAR,
    OP_WAIT,
    OP_SIGNAL,
    OP_FLUSH,
    OP_CREAR,
    OP_BORRAR,
}t_opcode;

typedef struct{
    t_opcode opcode;
    int argc;
    t_list* argv;
    char* puntero_string;
    char* puntero_string_aux;
}Instruccion;

static const int cant_argumentos_instruccion[] = {1, 1, 0, 3, 1, 1, 1, 2, 1};

Instruccion* parsear_linea(char*);
void destruir_instruccion(Instruccion*);
int ejecutar_linea(DTB*, char*, unsigned int);
t_opcode str_a_opcode(char*);

#endif //CPU_PARSER_H
