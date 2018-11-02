#ifndef CONFIG_TYPES_H_
#define CONFIG_TYPES_H_

	#include <commons/collections/list.h>
    #include <commons/config.h>
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
		CONFIG_cpu = 7,
		CONFIG_mdj = 3

	}cantidad_parametros; //cantidad de strings en su archivo configuracion

	typedef enum {
		RR,
		VRR,
		PROPIO
	}algoritmo_planificacion;

	typedef enum{
		SEG,
		TPI,
		SPA

	}modo_memoria;


	/*
	 * Definicion de los tipos de datos para leer los archivos de configuracion correctamente
	 */

	typedef struct{
		int puerto;
		algoritmo_planificacion algoritmo;
		int quantum;
		int multiprogramacion;
		int retardo;
		t_config* config;
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
		t_config* config;
	}cfg_elDiego;

	typedef struct {
		int puerto;
		modo_memoria modo;
		int max_linea;
		int tamanio;
		int tam_pagina;
		t_config* config;
	}cfg_fm9;

	typedef struct {
		char* ip_safa;
		int puerto_safa;
		char* ip_elDiego;
		int puerto_elDiego;
		char* ip_fm9;
		int puerto_fm9;
		int retardo;
		t_config* config;
	}cfg_cpu;

	typedef struct {
		int puerto;
		char* punto_montaje;
		int retardo;
		t_config* config;
	}cfg_mdj;


#endif /* CONFIG_TYPES_H_ */
