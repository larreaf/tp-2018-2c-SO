/*
 * mdj_functions.c
 *
 *  Created on: 29 sep. 2018
 *      Author: utnso
 */

#ifndef MDJ_FUNCTIONS_C_
#define MDJ_FUNCTIONS_C_

#include <stdio.h>
#include <stdlib.h>

#include <commons/string.h>
#include <commons/log.h>
#include <commons/bitarray.h>
#include <commons/collections/dictionary.h>
#include <commons/collections/node.h>

#include <readline/readline.h>

#include <ensalada/protocolo.h>
#include <ensalada/servidor.h>
#include <ensalada/mensaje.h>
#include <ensalada/validacion.h>

#include <pthread.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>

typedef struct {
	int t_operacion;
	char* path;
	int offset;
	int size;
	char* buffer;
	int cantidad_bytes;
} t_mdj_interface;

typedef struct{
	int tamanio_bloques;
	int cantidad_bloques;
}metadata_fifa;

/*
 * @NAME:	crear_data_mdj_operacion
 * @DESC:	desempaqueta un mensaje que se envía de manera ordenada y devuelve una estructura que la interfaz de mdj pueda entender
 * @RET:	retorna un struct t_mdj_interface común a todas las funciones
 */
t_mdj_interface* crear_data_mdj_operacion(MensajeDinamico* mensaje);

/*
 * @NAME:	validar_archivo
 * @DESC:	valida la existencia de un archivo en FIFA
 * @RET:	true si existe ; false si no existe
 */
bool validar_archivo(t_mdj_interface* data_operacion);

/*
 * @NAME:	crear_archivo
 * @DESC:	crea un archivo en FIFA
 * @RET:	retorna la cantidad de lineas asignadas en los bloques
 */
int crear_archivo(t_mdj_interface* data_operacion);

/*
 * @NAME:	obtener_datos
 * @DESC:	lee un archivo con un size y un offset
 * @RET:	retorna las lineas leidas
 */
char* obtener_datos(t_mdj_interface* data_operacion);

/*
 *********************************************************************************En construccion
 * @NAME:	guardar_datos
 * @DESC:	guarda datos en un archivo de FIFA
 * @RET:
 */
int guardar_datos(t_mdj_interface* data_operacion);

/*
 * @NAME:	borrar_archivo
 * @DESC:	elimina un archivo de FIFA
 * @RET:	void
 */
int borrar_archivo(t_mdj_interface* mdj_interface);

/*
 * @NAME:	levantar_metadata
 * @DESC:	lee el archivo Metadata.bin de fifa y lo guarda en una variable global
 * @RET:	void
 */
void levantar_metadata();

/*
 * @NAME:	bitmap_clean
 * @DESC:	Coloca todos los bits del bitmap en 0
 * @RET:	void
 */
void bitmap_clean();

/*
 * @NAME:	get_bitmap_to_string
 * @DESC:	genera un string para visualizar el bitmap
 * @RET:	char*
 */
char* get_bitmap_to_string();


#endif /* MDJ_FUNCTIONS_C_ */