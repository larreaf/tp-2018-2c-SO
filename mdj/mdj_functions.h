/*
 * mdj_functions.c
 *
 *  Created on: 29 sep. 2018
 *      Author: utnso
 */

#ifndef MDJ_FUNCTIONS_C_
#define MDJ_FUNCTIONS_C_
	#include <pthread.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <unistd.h>
	#include <pthread.h>
	#include <netinet/in.h>
	#include <commons/string.h>
	#include <commons/log.h>
	#include <commons/bitarray.h>
	#include <readline/readline.h>
	#include <ensalada/protocolo.h>
	#include <ensalada/servidor.h>
	#include <ensalada/mensaje.h>
	#include <ensalada/validacion.h>

	#include <sys/stat.h>
	#include <sys/mman.h>
	#include <fcntl.h>
	#include <unistd.h>
	#include <sys/types.h>


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

	bool validar_archivo(t_mdj_interface* data_operacion);

	void crear_archivo(t_mdj_interface* data_operacion);

	char* obtener_datos(t_mdj_interface* data_operacion);

	int guardar_datos(t_mdj_interface* data_operacion);

	void borrar_archivo(t_mdj_interface* mdj_interface);

	void levantar_metadata();

	void bitmap_clean();


#endif /* MDJ_FUNCTIONS_C_ */
