/*
 * mdj_functions.c
 *
 *  Created on: 29 sep. 2018
 *      Author: utnso
 */

#ifndef MDJ_FUNCTIONS_C_
#define MDJ_FUNCTIONS_C_
	#include <pthread.h>
	typedef struct {
		int t_operacion;
		char* path;
		int offset;
		int size;
		char* buffer;
		int cantidad_lineas;
	} t_mdj_interface;

	typedef struct{
		int tamanio_bloques;
		int cantidad_bloques;
	}metadata_fifa;

	void interface_mdj(MensajeDinamico* mensaje_dinamico);

	void validar_archivo(t_mdj_interface* data_operacion);

	void crear_archivo(t_mdj_interface* data_operacion);

	char* obtener_datos(t_mdj_interface* data_operacion);

	int guardar_datos(t_mdj_interface* data_operacion);
#endif /* MDJ_FUNCTIONS_C_ */
