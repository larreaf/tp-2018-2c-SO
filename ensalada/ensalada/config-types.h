#ifndef CONFIG_TYPES_H_
#define CONFIG_TYPES_H_

	#include <commons/collections/list.h>
	#define TAMANIO_STRING 25 // Tamanio maximo de los strings de los archivos de configuracion

	typedef enum {
			safa,
			elDiego,
			fm9,
			cpu,
			mdj
	}t_process; //indentifica el tipo de proceso

	typedef enum {
		CONFIG_safa = 5,
		CONFIG_elDiego = 8,
		CONFIG_fm9 = 5,
		CONFIG_cpu = 5,
		CONFIG_mdj = 3

	}cantidad_parametros; //cantidad de strings en su archivo configuracion

	typedef enum {
		RR,
		VRR,
		PROPIO
	}algoritmo_planificacion;

	typedef enum{
		SEG,
		PAGINV,
		SEGPAG

	}modo_memoria;

	/*
	 * Arrays de dos dimensiones para contrastar con los archivos de configuracion
	 * correspondientes y verificar si estan correctos.
	 */

	char CONFIGURACION_safa[CONFIG_safa][TAMANIO_STRING] = {
			"PUERTO",
			"ALGORITMO",
			"QUANTUM",
			"MULTIPROGRAMACION",
			"RETARDO_PLANIF"

	};

	char CONFIGURACION_elDiego[CONFIG_elDiego][TAMANIO_STRING] = {
			"PUERTO"
			"IP_SAFA",
			"PUERTO_SAFA",
			"IP_MDJ",
			"PUERTO_MDJ",
			"IP_FM9",
			"PUERTO_FM9",
			"TRANSFER_SIZE"
	};

	char CONFIGURACION_fm9[CONFIG_fm9][TAMANIO_STRING] = {
			"PUERTO",
			"MODO",
			"TAMANIO",
			"MAX_LINEA",
			"TAM_PAGINA"
	};

	char CONFIGURACION_cpu[CONFIG_cpu][TAMANIO_STRING] = {
			"IP_SAFA",
			"PUERTO_SAFA",
			"IP_DIEGO",
			"PUERTO_DIEGO",
			"RETARDO"
	};

	char CONFIGURACION_mdj[CONFIG_cpu][TAMANIO_STRING] = {
			"PUERTO",
			"PUNTO_MONTAJE",
			"RETARDO"
	};

	/*
	 * Definicion de los tipos de datos para leer los archivos de configuracion correctamente
	 */

	typedef struct{
		int puerto;
		algoritmo_planificacion algoritmo;
		int quantum;
		int multiprogramacion;
		int retardo;
	}cfg_safa;

	typedef struct{
		int puerto;
		char* 	ip_safa;
		int 	puerto_safa;
		char* 	ip_mdj;
		int 	puerto_mdj;
		char* 	ip_fm9;
		int 	puerto_fm9;
		int transfer_size;
	}cfg_elDiego;

	typedef struct {
		int puerto;
		modo_memoria modo;
		int max_linea;
		int tamanio;
		int tam_pagina;
	}cfg_fm9;

	typedef struct {
		char* ip_safa;
		int puerto_safa;
		char* ip_elDiego;
		int puerto_elDiego;
		int retardo;

	}cfg_cpu;

	typedef struct {
		int puerto;
		char* punto_montaje;
		int retardo;

	}cfg_mdj;


#endif /* CONFIG_TYPES_H_ */
