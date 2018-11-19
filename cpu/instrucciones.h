//
// Created by utnso on 30/09/18.
//

#ifndef CPU_INSTRUCCIONES_H
#define CPU_INSTRUCCIONES_H

#include <ensalada/dtb.h>
#include <ensalada/servidor.h>
#include <ensalada/mensaje.h>
#include <ensalada/protocolo.h>
#include "parser.h"

int in_abrir(DTB*, char*);
int in_close(DTB*, char*);
int in_flush(DTB*, char*);
int in_crear(int, char*, int);
int in_borrar(int, char*);
int in_asignar(DTB*, char*, int, char*);
int in_wait(DTB*, char*);
int in_signal(DTB*, char*);
int in_concentrar();

#endif //CPU_INSTRUCCIONES_H
