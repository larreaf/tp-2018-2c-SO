#include "validacion.h"

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
		"PUERTO",
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
		"IP_FM9",
		"PUERTO_FM9",
		"RETARDO"
};

char CONFIGURACION_mdj[CONFIG_mdj][TAMANIO_STRING] = {
		"PUERTO",
		"PUNTO_MONTAJE",
		"RETARDO"
};

algoritmo_planificacion string_to_alg(char* string){
	if(!strcmp(string,"RR"))
		return RR;
	if(!strcmp(string,"VRR"))
		return VRR;
	if(!strcmp(string,"PROPIO"))
		return PROPIO;
	return -1;
}
modo_memoria string_to_memory(char* string){
	if(!strcmp(string,"SEG"))
		return SEG;
	if(!strcmp(string,"PAGINV"))
		return PAGINV;
	if(!strcmp(string,"SEGPAG"))
		return SEGPAG;
	return -1;
}

void validar_parametros(int argc){
	if(argc < 2){
		printf("La cantidad de parametros es incorrecta!\n");
		exit(1);
	}
}

t_config* validar_config(char* path, t_process tipo_proceso){
	t_config* configuracion = config_create(path);
	int i = 0;
	char* mensaje_error;
	bool true_value;

	switch (tipo_proceso) {

		case safa :
			for(i = 0;i < CONFIG_cpu; i++){
				true_value = config_has_property(configuracion, CONFIGURACION_safa[i]);
				mensaje_error = string_from_format("Falta el valor %s",CONFIGURACION_safa[i]);
				atrapar_error(true_value,mensaje_error);
				free(mensaje_error);
			}
			break;

		case elDiego :
			for(i = 0;i < CONFIG_elDiego; i++){
				true_value = config_has_property(configuracion, CONFIGURACION_elDiego[i]);
				mensaje_error = string_from_format("Falta el valor %s",CONFIGURACION_elDiego[i]);
				atrapar_error(true_value,mensaje_error);
				free(mensaje_error);
			}
			break;

		case fm9 :
			for(i = 0;i < CONFIG_fm9; i++){
				true_value = config_has_property(configuracion, CONFIGURACION_fm9[i]);
				mensaje_error = string_from_format("Falta el valor %s",CONFIGURACION_fm9[i]);
				atrapar_error(true_value,mensaje_error);
				free(mensaje_error);
			}

			break;

		case cpu :
			for(i = 0;i < CONFIG_cpu; i++){
				true_value = config_has_property(configuracion, CONFIGURACION_cpu[i]);
				mensaje_error = string_from_format("Falta el valor %s",CONFIGURACION_cpu[i]);
				atrapar_error(true_value,mensaje_error);
				free(mensaje_error);
			}
			break;

		case mdj :
			for(i = 0;i < CONFIG_mdj; i++){
				true_value = config_has_property(configuracion, CONFIGURACION_mdj[i]);
				mensaje_error = string_from_format("Falta el valor %s",CONFIGURACION_mdj[i]);
				atrapar_error(true_value,mensaje_error);
				free(mensaje_error);
			}
			break;

		default :
			atrapar_error(0, "Tipo de proceso incorrecto\n");

	}
	printf("Archivo de configuracion correcto!\n");

	return configuracion;
}

void* asignar_config(t_config * archivo_config,t_process tipo_proceso){
	void *retorno;
	cfg_safa* retorno_safa;
	cfg_elDiego* retorno_elDiego;
	cfg_fm9* retorno_fm9;
	cfg_cpu* retorno_cpu;
	cfg_mdj* retorno_mdj;

	switch (tipo_proceso) {

		case safa :
			retorno_safa = malloc(sizeof(cfg_safa));

			retorno_safa->puerto = config_get_int_value(archivo_config,CONFIGURACION_safa[0]);
			retorno_safa->algoritmo = string_to_alg(config_get_string_value(archivo_config,CONFIGURACION_safa[1]));
			retorno_safa->quantum = config_get_int_value(archivo_config,CONFIGURACION_safa[2]);
			retorno_safa->multiprogramacion = config_get_int_value(archivo_config, CONFIGURACION_safa[3]);
			retorno_safa->retardo = config_get_int_value(archivo_config, CONFIGURACION_safa[4]);
			retorno = retorno_safa;

			break;

		case elDiego :
			retorno_elDiego = malloc(sizeof(cfg_elDiego));

			retorno_elDiego->puerto = config_get_int_value(archivo_config, CONFIGURACION_elDiego[0]);
			retorno_elDiego->ip_safa = config_get_string_value(archivo_config, CONFIGURACION_elDiego[1]);
			retorno_elDiego->puerto_safa = config_get_int_value(archivo_config, CONFIGURACION_elDiego[2]);
			retorno_elDiego->ip_mdj = config_get_string_value(archivo_config, CONFIGURACION_elDiego[3]);
			retorno_elDiego->puerto_mdj = config_get_int_value(archivo_config, CONFIGURACION_elDiego[4]);
			retorno_elDiego->ip_fm9 = config_get_string_value(archivo_config, CONFIGURACION_elDiego[5]);
			retorno_elDiego->puerto_fm9 = config_get_int_value(archivo_config, CONFIGURACION_elDiego[6]);
			retorno_elDiego->transfer_size = config_get_int_value(archivo_config, CONFIGURACION_elDiego[7]);

			retorno = retorno_elDiego;
			break;

		case fm9 :
			retorno_fm9 = malloc(sizeof(cfg_fm9));

			retorno_fm9->puerto = config_get_int_value(archivo_config,CONFIGURACION_fm9[0]);
			retorno_fm9->modo = string_to_memory(config_get_string_value(archivo_config,CONFIGURACION_fm9[1]));
			retorno_fm9->tamanio = config_get_int_value(archivo_config,CONFIGURACION_fm9[2]);
			retorno_fm9->max_linea = config_get_int_value(archivo_config,CONFIGURACION_fm9[3]);
			retorno_fm9->tam_pagina = config_get_int_value(archivo_config,CONFIGURACION_fm9[4]);

			retorno = retorno_fm9;
			break;

		case cpu :
			retorno_cpu = malloc(sizeof(cfg_cpu));

			retorno_cpu->ip_safa = config_get_string_value(archivo_config,CONFIGURACION_cpu[0]);
			retorno_cpu->puerto_safa = config_get_int_value(archivo_config,CONFIGURACION_cpu[1]);
			retorno_cpu->ip_elDiego = config_get_string_value(archivo_config,CONFIGURACION_cpu[2]);
			retorno_cpu->puerto_elDiego = config_get_int_value(archivo_config,CONFIGURACION_cpu[3]);
			retorno_cpu->ip_fm9= config_get_string_value(archivo_config,CONFIGURACION_cpu[4]);
			retorno_cpu->puerto_fm9 = config_get_int_value(archivo_config,CONFIGURACION_cpu[5]);
			retorno_cpu->retardo = config_get_int_value(archivo_config,CONFIGURACION_cpu[6]);

			retorno = retorno_cpu;
			break;

		case mdj :
			retorno_mdj = malloc(sizeof(cfg_mdj));

			retorno_mdj->puerto = config_get_int_value(archivo_config,CONFIGURACION_mdj[0]);
			retorno_mdj->punto_montaje = config_get_string_value(archivo_config,CONFIGURACION_mdj[1]);
			retorno_mdj->retardo = config_get_int_value(archivo_config,CONFIGURACION_mdj[2]);

			retorno = retorno_mdj;
			break;

	}
	return retorno;
}

void destroy_cfg (void* cfg, t_process tipo){
	switch (tipo) {

			case safa :
				free(cfg);
				break;

			case elDiego :
			//	free(((cfg_elDiego*)(cfg))->ip_safa);
			//	free(((cfg_elDiego*)(cfg))->ip_mdj);
			//	free(((cfg_elDiego*)(cfg))->ip_fm9);
				free(cfg);
				break;

			case fm9 :
				free(cfg);
				break;

			case cpu :
			//	free(((cfg_cpu*)(cfg))->ip_safa);
			//	free(((cfg_cpu*)(cfg))->ip_elDiego);
				free(cfg);
				break;

			case mdj :
				//free((void*)((cfg_mdj*)(cfg))->punto_montaje);
				free(cfg);
				break;
			default:
				atrapar_error(0,"Error en tipo");
				break;

		}
}


void atrapar_error(bool valor, char* mensaje_error){
	if(valor == 0){
		printf("Error: %s",mensaje_error);
		free(mensaje_error);
		exit(1);
	}
}
