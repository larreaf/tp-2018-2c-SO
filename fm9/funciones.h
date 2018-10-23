/*
 * FM9_functions.h
 *
 *  Created on: 23/10/2018
 *
 */

#ifndef FM9_FUNCIONES_C_
#define FM9_FUNCIONES_C_

#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <sys/select.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <ensalada/com.h>
#include <ensalada/protocolo.h>
#include <ensalada/servidor.h>
#include <ensalada/validacion.h>

//Estructuras

typedef struct{
    int id;
    char* instruccion;
    char* contenido;
}DiegoAFM9;

//Funciones

void inicializar_storage(t_log *logger,int tamanioMemoria);

void* asignar_memoria(t_log *logger, int tamanioMemoria);

void desempaquetar_mensaje_diego_a_fm9(t_log *logger, MensajeDinamico* mensaje, DiegoAFM9* DiegoAFM9);

void segmentacion_pura();

void tabla_de_paginas_invertidas();

void segmentacion_paginada();

void elegir_tipo_funcionamiento_memoria(t_log *logger,char unTipoFuncionamiento);

char* buscar_en_memoria(t_log *logger, DiegoAFM9* datos_fm9);

int escribir_en_memoria(t_log *logger, DiegoAFM9* datos_fm9);

int modificar_en_memoria(t_log *logger, DiegoAFM9* datos_fm9);

void destruir_storage();

void cerrar_fm9(t_log* logger, cfg_fm9* configuracion, ConexionesActivas server);

#endif /* FM9_FUNCIONES_C_ */
